# Solar Powered Camera with Selective Area Intrusion Detection System

## Overview

This project is a Raspberry Pi based Solar Powered Camera with Selective Area Intrusion Detection System designed to monitor a secured zone, detect unauthorized movement, capture evidence, and notify the user by email.

I designed the system to use a PIR sensor as the first trigger so that the camera only comes on when motion is detected near the protected area. Once the camera is active, the system monitors a user-defined zone on the live stream. If an intruder crosses the marked zone, the system changes the zone indicator from green to red, captures snapshots, records a video of the intrusion, sounds the alarm buzzer, stores all evidence inside an event folder, and sends an email alert with attached evidence.

The project is structured so that zone setup is handled separately from the main intrusion logic. This makes it easy to redraw the zone whenever needed without modifying the main security system.

The system includes:

- Raspberry Pi based controller
- PIR-triggered camera activation
- Separate zone drawing setup
- Zone-based intrusion confirmation
- Green to red zone status display
- Automatic image capture per zone crossing
- Automatic event video recording
- Event-based evidence storage
- Indoor and outdoor buzzer alarm
- Email alert with evidence attachment
- Flask-based live video streaming
- Non-blocking event timing logic

---

# Features

- PIR sensor wake-up logic
- Camera remains off until PIR detects motion
- Separate zone setup script
- Rectangle zone monitoring
- Zone turns green normally and red when crossed
- Multiple zone-crossing image capture during one event
- One event video per PIR activity session
- Event evidence stored in organized folders
- Email alert when intrusion occurs
- Large video files still saved locally even if not attached to email
- Live MJPEG browser stream
- Non-blocking timing structure for smooth streaming
- Automatic event reset and re-arming

---

# Hardware Used

## Main Controller

- Raspberry Pi 4 / Raspberry Pi 5

## Camera

- Raspberry Pi Camera Module

## Sensors

- PIR motion sensor

## Alarm Outputs

- Indoor buzzer
- Outdoor buzzer

## Communication

- Local network browser access through Flask web streaming
- Gmail SMTP email notification

---

# Project Structure

```text
Intruder_Alarm_System/
│
├── main_intrusion.py
├── zone_setup.py
├── zone.json
│
├── events/
│   ├── event_1/
│   │   ├── images/
│   │   └── videos/
│   ├── event_2/
│   │   ├── images/
│   │   └── videos/
│
├── logs/
│   └── system.log
│
├── config/
│   └── settings.json
```

---

# System Operation

## General Workflow

1. The system stays idle while waiting for PIR detection.
2. Once the PIR sensor detects motion, the camera starts and the live stream becomes active.
3. The system begins monitoring the saved zone from `zone.json`.
4. When motion crosses the protected zone:
   - the zone changes from green to red
   - a snapshot sequence starts
   - the buzzer alarm sounds
   - video recording starts for the event
5. If the protected zone is crossed again during the same PIR activity session, another image sequence is captured.
6. All images and video for that PIR session remain inside the same event folder.
7. When the area becomes inactive for the configured delay period, the event ends, the video is saved, the buzzer stops, and the system resets to wait for the next PIR trigger.

---

# Zone Setup

The project uses a separate script for drawing the detection zone.

## Zone Mapping Script

File:

```text
zone_setup.py
```

This script is used only when I want to define or redraw the protected rectangle.

## Zone Setup Behavior

- Opens the camera preview
- Allows me to draw a rectangle on the live view
- Saves the selected rectangle into `zone.json`
- Exits after the zone is saved

This keeps the main intrusion system clean and avoids accidental zone editing during operation.

---

# Main Intrusion Logic

The main system file is:

```text
main_intrusion.py
```

This script handles:

- PIR trigger monitoring
- Camera start and stop control
- Zone crossing detection
- Image capture
- Video recording
- Alarm buzzer control
- Event folder creation
- Email notification
- Browser live stream

---

# Event Logic

## PIR Trigger

The PIR sensor acts as the primary wake-up trigger.

When PIR detects motion:

- The camera is turned on
- Live streaming becomes available
- The system begins watching the protected zone

If PIR is not triggered:

- The system remains idle
- The camera stays off
- The stream shows waiting status

---

## Zone Crossing Confirmation

The PIR sensor alone does not count as an intrusion event.

The event is only confirmed when movement crosses the saved zone area on the camera frame.

This improves reliability by ensuring that nearby motion outside the protected area does not immediately count as an intrusion.

---

## Image Capture Per Zone Crossing

Each fresh zone crossing triggers an image sequence.

Current logic:

- 2 images are captured per valid zone crossing
- The 2 images have a 1 second interval
- A new zone crossing is only counted again after the zone has remained clear for a short delay

This means that if the intruder crosses the zone several times during one PIR session, the saved image count can be more than just the initial 2 images.

---

## Video Recording

Video recording is event-based.

Behavior:

- One video file is created for each PIR event session
- The same event video continues while the event remains active
- The video is saved locally inside the event folder
- Even if the video is too large to attach to email, it is still recorded and saved

Video location example:

```text
events/event_1/videos/
```

---

## Alarm Behavior

The buzzer alarm uses its normal beep pattern.

Behavior:

- Alarm starts when an intrusion event begins
- Alarm continues while valid zone-crossing activity is still happening
- Alarm stops only after there has been no more zone crossing for the configured delay period

This makes the system behave like a true intrusion alarm instead of stopping too early.

---

# Live Streaming

The project uses Flask to stream the camera feed in the browser.

The stream provides:

- Live camera feed
- Waiting screen when idle
- Zone rectangle overlay
- Green zone during normal monitoring
- Red zone when the protected area is crossed

## Browser Access

The Flask server runs on port 5000.

Example access:

```text
http://<raspberry_pi_ip>:5000
```

To check the Raspberry Pi IP address:

```bash
hostname -I
```

---

# Event Folder Structure

Every PIR intrusion session creates one event folder.

Example:

```text
events/
└── event_1/
    ├── images/
    └── videos/
```

## Images Folder

Stores all snapshots taken during zone crossings for that event.

## Videos Folder

Stores the single video file recorded for that event.

This makes it easy to review evidence per intrusion session.

---

# Email Alert

The system sends an email notification when an intrusion event occurs.

## Email Behavior

- Email is sent through Gmail SMTP
- Evidence images are attached
- Video is attached only if it is small enough
- If video is too large, the email still sends and the video remains saved locally

## Gmail Requirement

The sender Gmail account must use:

- 2-Step Verification
- App Password

The normal Gmail password must not be used. Before running the system, copy `code/.env.example` to `code/.env` and set the local values:

```dotenv
EMAIL_FROM=sender@example.com
EMAIL_TO=recipient@example.com
EMAIL_PASSWORD=provider-app-password
SMTP_HOST=smtp.gmail.com
SMTP_PORT=465
```

`main_intrusion.py` reads this file at startup. The populated `.env` is ignored by Git; do not place credentials in Python source, a README, screenshots, or version control. Environment variables supplied by a service manager take precedence over `.env` values.

---

# Pin Configuration

## PIR Sensor

| Signal | GPIO |
| ------ | ---- |
| PIR OUT | GPIO17 |

## Buzzers

| Device | GPIO |
| ------ | ---- |
| Indoor Buzzer | GPIO18 |
| Outdoor Buzzer | GPIO23 |

---

# Software Requirements

The project uses Python and Raspberry Pi camera libraries.

Typical packages include:

- OpenCV
- NumPy
- Flask
- Picamera2
- gpiozero

Example installation:

```bash
sudo apt update
sudo apt install python3-opencv python3-flask python3-gpiozero python3-numpy python3-picamera2 -y
```

---

# Running the Project

## Step 1 — Enter the Project Folder

```bash
cd code
cp .env.example .env
chmod 600 .env
```

Edit `.env` with the sender, recipient, and provider-specific app password before continuing.

## Step 2 — Draw the Zone

```bash
python3 zone_setup.py
```

## Step 3 — Start the Main System

```bash
python3 main_intrusion.py
```

## Step 4 — Open the Live Stream

```text
http://<raspberry_pi_ip>:5000
```

---

# Operating Guide

## Normal Use

1. Power the Raspberry Pi and connect the camera, PIR sensor, and buzzers.
2. Run the zone setup script if the protected zone needs to be changed.
3. Start the main intrusion script.
4. Open the web stream if monitoring is needed.
5. The system waits for PIR motion.
6. Once PIR is triggered, the camera starts and the zone becomes active.
7. If the zone is crossed, the system captures evidence, sounds the alarm, records video, and sends email.

---

## Redrawing the Zone

Whenever I want to redraw the protected area:

1. Stop the main system if necessary
2. Run:

```bash
python3 zone_setup.py
```

3. Draw the new zone
4. Save and exit
5. Restart the main intrusion script

---

# Troubleshooting

## Camera Does Not Start

- Confirm the Raspberry Pi camera is connected properly
- Confirm Picamera2 is installed
- Test the camera separately with:

```bash
rpicam-hello
```

---

## Browser Stream Does Not Open

- Make sure `main_intrusion.py` is running
- Confirm Raspberry Pi and browser device are on the same network
- Check the Pi IP address with:

```bash
hostname -I
```

- Open:

```text
http://<raspberry_pi_ip>:5000
```

---

## Email Does Not Send

- Confirm internet is available on the Raspberry Pi
- Confirm Gmail app password is correct
- Confirm 2-Step Verification is enabled on the sender Gmail account
- Do not use the normal Gmail password
- Check the terminal for SMTP error output

---

## Zone Turns Red Too Early

- Redraw the zone carefully using `zone_setup.py`
- Make sure the zone only covers the protected area
- Avoid placing the zone too close to moving objects like curtains or reflective surfaces

---

## Video Not Attached to Email

- This can happen if the video is too large for email
- The video is still saved locally in the event folder
- Check:

```text
events/event_x/videos/
```

---

# Safety Notes

- Use a stable power supply for the Raspberry Pi
- Keep buzzer wiring secure
- Avoid exposing the Pi and camera to outdoor weather unless enclosed
- Do not store sensitive email passwords directly in public repositories
- Replace passwords with placeholders before uploading to GitHub

---

# Future Improvements

- Attach the latest image preview in richer email format
- Upload large videos automatically to cloud storage
- Add date and time stamp overlay on all evidence
- Add Telegram or WhatsApp alert integration
- Add face recognition or person detection
- Add multiple protected zones
- Add event log summary dashboard

---

## Security and local configuration

Never commit populated `.env` files, Wi-Fi credentials, passwords, API keys, SMTP credentials, broker tokens, private certificates, or production connection strings. Keep them in an ignored local configuration file or approved secret store, and commit only a redacted `.env.example` when configuration is needed. Replace development access-point credentials before deployment and limit local administration to trusted networks.

# Author

Egbe Raymond

Electrical / Electronic Engineer  
Embedded Systems Developer  
Full Stack Developer

---

## License

Released under the [MIT License](LICENSE).
