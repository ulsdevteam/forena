<?php
/*
 * Sample Repository configuration file
 */

/*
 * Security provider:  Specify the class name that is used to provide security  
 */
$security_provider = 'FrxDrupalSecurity';

/*
 * Data provider: 
 * Specify the class name that will be used to interpret data block files.   
 * Note that data blocks in a repository  
 * 
 */

$data_engine = 'FrxFileEngine'; 

/*
 * URI: 
 * The format of the uri depends on the type of data engine that's being used. 
 * In database engines it might be the connection string to the db.  In the file 
 * engine it would be the path to the directory containting the files
 */
$uri = 'file://'. drupal_get_path('module','forena') . '/repos/sample';

/**
 * Uncomment the following line to specify a particular path in which to save reports. 
 * The default value would be controlled by Forena configuration page. If you'd like users 
 * to be able to define their own reports in this repository then you need to make sure that this 
 * is writable by the web user (e.g. apache).  
 */
// $report_path = 'sites/files/reports' 
