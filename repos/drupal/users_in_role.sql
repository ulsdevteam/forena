--ACCESS=administer users
SELECT * FROM role r JOIN users_roles ur ON r.rid=ur.rid
  WHERE r.rid = :role