--ACCESS=access administration pages
SELECT name, mail, login, status FROM users 
  WHERE status=1 order by name