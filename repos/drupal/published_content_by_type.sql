--ACCESS=access content
SELECT nid, type, title, uid, sticky, promote from node WHERE type=:content_type
  and status=1 
  ORDER BY title
