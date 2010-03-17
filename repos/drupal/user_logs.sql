--ACCESS=access content
SELECT u.uid, u.name, timestamp, type, severity, wid from watchdog w JOIN users u on u.uid=w.uid 
  WHERE u.name=:name