import './styles.css';
import './dashboard-fixes.css';
export const metadata = { title: 'Crop Water Stress Monitor', description: 'Device monitoring and water-stress risk dashboard' };
export default function RootLayout({ children }: { children: React.ReactNode }) { return <html lang="en"><body>{children}</body></html>; }
