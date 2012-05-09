--ACCESS=administer content
SELECT * FROM node 
  ORDER BY sticky DESC, created 
LIMIT COALESCE(CAST(:limit AS integer), 100)