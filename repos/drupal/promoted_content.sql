--ACCESS=access content
SELECT * FROM node 
  WHERE promote=1
    AND status=1
  ORDER BY sticky DESC, created 
LIMIT COALESCE(CAST(:limit AS integer), 10)
  
  