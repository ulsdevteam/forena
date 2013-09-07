--ACCESS=access content
SELECT nid, type, title, uid, sticky, promote from node WHERE type=:content_type
  and status=1 
  ORDER BY title
--IF=:limit
  LIMIT :limit
--INFO
; This demonstrates loading node entities.
entity_type = node
entity_id = nid
; the following demonstrates an optional limit clause that can be used with SQL
type[limit] = int