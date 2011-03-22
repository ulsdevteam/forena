--ACCESS=administer permissions
SELECT * FROM role_permission P
  WHERE p.rid = :role