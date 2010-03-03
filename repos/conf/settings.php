<?php
// THis is a sample repository settings file
// This would be where you put your connection strings to the database
$securityProvider = 'FrxDrupalAuth'; 
$dataProvider = 'FrxFileEngine'; 
$path = drupal_get_path('forena', 'module'); 
$uri = 'file://' . $path;
