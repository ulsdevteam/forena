--ACCESS=access demo reports
select * from user_distribution WHERE 
  COALESCE( :state, state) = state 
