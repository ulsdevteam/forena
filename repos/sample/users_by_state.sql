--ACCESS=access demo reports
select 
  code AS state,
  name,
  total, 
  CASE 
    WHEN total>10000 THEN 'good'
    WHEN total>5000 THEN 'average'
    ELSE 'poor'
  end AS classification
FROM states s join 
  (SELECT state,sum(users) as total from user_distribution 
    GROUP BY state) su
  ON su.state=s.code
ORDER BY total desc

