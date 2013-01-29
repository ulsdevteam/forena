--ACCESS=access demo reports
select * from states
--IF=:state
WHERE code=:state
--END
ORDER BY name


