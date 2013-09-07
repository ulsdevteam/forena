--ACCESS=access content
SELECT nid, type, title, uid, sticky, promote from node WHERE type=:content_type
  and status=1 
  ORDER BY title
--INFO
; This demonstrates loading node entities.
entity_type = node
entity_id = nid
type[limit] = int
