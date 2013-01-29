--ACCESS=access demo reports
select * from user_distribution WHERE 
  state in (:state)
