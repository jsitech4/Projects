import cv2
import json
import numpy as np
import time
import os
from datetime import datetime
from flask import Flask, Response
from picamera2 import Picamera2
from gpiozero import MotionSensor, Buzzer
import threading
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart
from email.mime.base import MIMEBase
from email import encoders

ZONE_FILE = "zone.json"
EVENTS_ROOT = "events"


def load_local_env(path=".env"):
    """Load simple KEY=VALUE pairs from a local, ignored .env file.

    Existing process environment values take precedence so deployments can use
    their normal secret manager instead of a file.
    """
    if not os.path.exists(path):
        return
    with open(path, "r", encoding="utf-8") as env_file:
        for raw_line in env_file:
            line = raw_line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, value = line.split("=", 1)
            os.environ.setdefault(key.strip(), value.strip().strip('"').strip("'"))


load_local_env()

PIR_PIN = 17
BUZZER_INDOOR = 18
BUZZER_OUTDOOR = 23

CAMERA_OFF_DELAY = 20
ZONE_END_DELAY = 3
CROSSING_REARM_DELAY = 3
SNAPSHOT_INTERVAL = 1
MAX_EMAIL_ATTACHMENT_BYTES = 24 * 1024 * 1024

EMAIL_FROM = os.getenv("EMAIL_FROM", "")
EMAIL_TO = os.getenv("EMAIL_TO", "")
EMAIL_PASSWORD = os.getenv("EMAIL_PASSWORD", "")
SMTP_HOST = os.getenv("SMTP_HOST", "smtp.gmail.com")
SMTP_PORT = int(os.getenv("SMTP_PORT", "465"))

os.makedirs(EVENTS_ROOT, exist_ok=True)
app = Flask(__name__)

pir = MotionSensor(PIR_PIN)
buzzer1 = Buzzer(BUZZER_INDOOR)
buzzer2 = Buzzer(BUZZER_OUTDOOR)
picam2 = Picamera2()

zone = None
prev_gray = None

camera_active = False
event_active = False
event_open_for_current_pir = False

last_pir_time = 0
last_zone_motion_time = 0

recording = False
video_writer = None
video_started = False

event_count = 0
current_event_folder = None
video_folder = None
images_folder = None
current_video_path = None
current_image_paths = []

buzzer_active = False

crossing_armed = True
zone_clear_since = None

snapshot_sequence_active = False
snapshot_sequence_id = 0
snapshot1_taken = False
snapshot2_taken = False
snapshot2_due_time = 0

def load_zone():
    try:
        with open(ZONE_FILE, "r") as f:
            return json.load(f)
    except:
        return None

zone = load_zone()

def attach_file(msg, filepath):
    with open(filepath, "rb") as f:
        part = MIMEBase("application", "octet-stream")
        part.set_payload(f.read())
    encoders.encode_base64(part)
    part.add_header("Content-Disposition", f'attachment; filename="{os.path.basename(filepath)}"')
    msg.attach(part)

def send_event_email(image_paths, video_path, event_id):
    try:
        if not all((EMAIL_FROM, EMAIL_TO, EMAIL_PASSWORD)):
            raise RuntimeError("Email is not configured; set EMAIL_FROM, EMAIL_TO, and EMAIL_PASSWORD in .env")
        msg = MIMEMultipart()
        body_lines = [f"Intruder detected. Event ID: {event_id}"]

        msg["Subject"] = f"Intruder Alert - Event {event_id}"
        msg["From"] = EMAIL_FROM
        msg["To"] = EMAIL_TO

        for image_path in image_paths:
            if os.path.exists(image_path):
                attach_file(msg, image_path)

        if video_path and os.path.exists(video_path):
            video_size = os.path.getsize(video_path)
            if video_size <= MAX_EMAIL_ATTACHMENT_BYTES:
                attach_file(msg, video_path)
            else:
                body_lines.append(f"Video saved locally but not attached because it is too large ({video_size} bytes).")

        msg.attach(MIMEText("\n".join(body_lines), "plain"))

        server = smtplib.SMTP_SSL(SMTP_HOST, SMTP_PORT, timeout=20)
        server.login(EMAIL_FROM, EMAIL_PASSWORD)
        server.sendmail(EMAIL_FROM, [EMAIL_TO], msg.as_string())
        server.quit()

        print(f"Event {event_id} email sent successfully")

    except Exception as e:
        print("Email error:", e)

def send_event_email_async(image_paths, video_path, event_id):
    threading.Thread(
        target=send_event_email,
        args=(list(image_paths), video_path, event_id),
        daemon=True
    ).start()

def buzzer_loop():
    global buzzer_active
    while buzzer_active:
        buzzer1.on()
        buzzer2.on()
        time.sleep(0.3)
        buzzer1.off()
        buzzer2.off()
        time.sleep(0.3)

def start_buzzer():
    global buzzer_active
    if not buzzer_active:
        buzzer_active = True
        threading.Thread(target=buzzer_loop, daemon=True).start()

def stop_buzzer():
    global buzzer_active
    buzzer_active = False
    buzzer1.off()
    buzzer2.off()

def get_next_event_number():
    if not os.path.exists(EVENTS_ROOT):
        return 1
    max_num = 0
    for name in os.listdir(EVENTS_ROOT):
        if name.startswith("event_"):
            try:
                n = int(name.split("_")[1])
                if n > max_num:
                    max_num = n
            except:
                pass
    return max_num + 1

def create_event():
    global event_count, current_event_folder, video_folder, images_folder
    global current_video_path, current_image_paths
    global video_started, recording, video_writer

    if event_count == 0:
        event_count = get_next_event_number() - 1

    event_count += 1
    current_event_folder = os.path.join(EVENTS_ROOT, f"event_{event_count}")
    video_folder = os.path.join(current_event_folder, "videos")
    images_folder = os.path.join(current_event_folder, "images")

    os.makedirs(video_folder, exist_ok=True)
    os.makedirs(images_folder, exist_ok=True)

    current_video_path = None
    current_image_paths = []

    video_started = False
    recording = False
    video_writer = None

def save_snapshot(frame, crossing_id, shot_no):
    global current_image_paths
    filename = datetime.now().strftime(f"%Y%m%d_%H%M%S_cross{crossing_id}_img{shot_no}.jpg")
    path = os.path.join(images_folder, filename)
    cv2.imwrite(path, frame)
    current_image_paths.append(path)
    print(f"Snapshot saved: {path}")

def start_video():
    global video_writer, recording, video_started, current_video_path

    if not video_started:
        current_video_path = os.path.join(
            video_folder,
            datetime.now().strftime("%Y%m%d_%H%M%S.avi")
        )
        fourcc = cv2.VideoWriter_fourcc(*"XVID")
        video_writer = cv2.VideoWriter(current_video_path, fourcc, 20.0, (640, 480))

        if not video_writer.isOpened():
            print(f"Failed to open video writer for {current_video_path}")
            video_writer = None
            recording = False
            video_started = False
            return

        recording = True
        video_started = True
        print(f"Event {event_count} video started: {current_video_path}")

def stop_video():
    global video_writer, recording
    if video_writer is not None:
        video_writer.release()
        video_writer = None
    if recording:
        print(f"Event {event_count} video stopped")
    recording = False

def start_snapshot_sequence():
    global snapshot_sequence_active, snapshot_sequence_id
    global snapshot1_taken, snapshot2_taken, snapshot2_due_time

    snapshot_sequence_active = True
    snapshot_sequence_id += 1
    snapshot1_taken = False
    snapshot2_taken = False
    snapshot2_due_time = time.time() + SNAPSHOT_INTERVAL

    print(f"Crossing {snapshot_sequence_id} snapshot sequence started")

def process_snapshot_sequence(frame):
    global snapshot_sequence_active, snapshot1_taken, snapshot2_taken

    if not snapshot_sequence_active:
        return

    if not snapshot1_taken:
        save_snapshot(frame, snapshot_sequence_id, 1)
        snapshot1_taken = True

    if snapshot1_taken and not snapshot2_taken and time.time() >= snapshot2_due_time:
        save_snapshot(frame, snapshot_sequence_id, 2)
        snapshot2_taken = True
        snapshot_sequence_active = False
        print(f"Crossing {snapshot_sequence_id} snapshot sequence completed")

def pir_triggered():
    global camera_active, prev_gray, last_pir_time

    last_pir_time = time.time()

    if not camera_active:
        picam2.configure(
            picam2.create_preview_configuration(
                main={"format": "RGB888", "size": (640, 480)}
            )
        )
        picam2.start()
        camera_active = True
        prev_gray = None
        print("PIR → Camera ON")

pir.when_motion = pir_triggered

def generate_frames():
    global camera_active, prev_gray
    global event_active, event_open_for_current_pir
    global last_pir_time, last_zone_motion_time
    global recording
    global crossing_armed, zone_clear_since

    while True:
        if camera_active:
            frame = picam2.capture_array()

            gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
            gray = cv2.GaussianBlur(gray, (21, 21), 0)

            crossed = False

            if prev_gray is not None and zone:
                delta = cv2.absdiff(prev_gray, gray)
                thresh = cv2.threshold(delta, 25, 255, cv2.THRESH_BINARY)[1]
                thresh = cv2.dilate(thresh, None, iterations=2)
                contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

                for c in contours:
                    if cv2.contourArea(c) < 800:
                        continue

                    x, y, w, h = cv2.boundingRect(c)
                    zx1, zy1, zx2, zy2 = zone

                    if (
                        x + w > min(zx1, zx2)
                        and y + h > min(zy1, zy2)
                        and x < max(zx1, zx2)
                        and y < max(zy1, zy2)
                    ):
                        crossed = True
                        break

            prev_gray = gray

            if pir.motion_detected:
                last_pir_time = time.time()

            if crossed:
                last_zone_motion_time = time.time()
                zone_clear_since = None
            else:
                if zone_clear_since is None:
                    zone_clear_since = time.time()
                elif time.time() - zone_clear_since >= CROSSING_REARM_DELAY:
                    crossing_armed = True

            if crossed and crossing_armed:
                crossing_armed = False

                if not event_open_for_current_pir:
                    event_active = True
                    event_open_for_current_pir = True
                    last_zone_motion_time = time.time()

                    create_event()
                    start_buzzer()
                    start_video()

                    print(f"Event {event_count} started")

                if event_active:
                    start_snapshot_sequence()

            if event_active:
                process_snapshot_sequence(frame)

                if recording and video_writer is not None:
                    video_writer.write(frame)

                if (
                    not pir.motion_detected
                    and time.time() - last_zone_motion_time >= ZONE_END_DELAY
                ):
                    stop_video()
                    stop_buzzer()
                    event_active = False

                    send_event_email_async(
                        current_image_paths,
                        current_video_path,
                        event_count
                    )

                    print(f"Event {event_count} completed")

            if (
                not pir.motion_detected
                and not event_active
                and time.time() - last_pir_time >= CAMERA_OFF_DELAY
            ):
                stop_video()
                stop_buzzer()
                picam2.stop()
                camera_active = False
                prev_gray = None
                event_open_for_current_pir = False
                crossing_armed = True
                zone_clear_since = None
                print("System reset → waiting for PIR")

            if zone:
                color = (0, 0, 255) if crossed else (0, 255, 0)
                cv2.rectangle(frame, (zone[0], zone[1]), (zone[2], zone[3]), color, 2)

        else:
            frame = np.zeros((480, 640, 3), dtype=np.uint8)
            cv2.putText(
                frame,
                "Waiting for PIR...",
                (100, 240),
                cv2.FONT_HERSHEY_SIMPLEX,
                1,
                (0, 255, 0),
                2,
            )

        ret, buffer = cv2.imencode(".jpg", frame)
        yield (
            b"--frame\r\n"
            b"Content-Type: image/jpeg\r\n\r\n" + buffer.tobytes() + b"\r\n"
        )

@app.route("/")
def video_feed():
    return Response(
        generate_frames(),
        mimetype="multipart/x-mixed-replace; boundary=frame",
    )

if __name__ == "__main__":
    print("System ready → waiting for PIR")
    app.run(host="0.0.0.0", port=5000, threaded=True)
