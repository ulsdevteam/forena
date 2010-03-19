--ACCESS=access administration pages
SELECT u.uid, u.name, timestamp, location, type, severity, wid from watchdog w JOIN users u on u.uid=w.uid 
  WHERE u.name=:name ORDER BY timestamp desc