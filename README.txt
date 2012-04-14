Forena Reports

Enterprise Reporting module that provides a mechanism for writing reports 
in a modified html syntax.  This module leverages XML, and for this 
reason required PHP 5.2 in order to run correctly. 

Installation
------------

Install in your sites/all/modules folder per the normal module installation 
method. 

Next visit the Site configuration->Forena page and configure the path where 
reports wil be saved.  This path should be in a place that is writable by your 
the user context which runs your web server.  When in doubt, specify the report
location within your files directory. 

After saving your settings, visit your my reports page to see help reports, 
sample reports, as well as some useful canned drupal administration reports. 
The provided sample reports also serve as documentation for the product. 

Creating your own data blocks
------------------------------
If you are interested in creating data_blocks (SQL files for use in reports).  
you will need to modify your sites settings.php file to create custom data 
repositories.  The following code should be added to your drupal sites settings.php
and modified to suite your needs: 

// Always include the global line
global $_forena_repositories;
// The following data would be referenced in a report as mysite/sqlfilename
$_forena_repositories['mysite'] = array(
    'path' => 'data_blocks/site_sql',             //Indicate the directory that will contain the sql files.  You'll need to create this directory
    'title' => 'Custom data blocks',              //Only used in error messages. 
    'data provider' => 'FrxDrupal',               //Use the drupal connection to connect to the database
    'debug' => @$_GET['debug'],                   //This is a technique I use to help debug sql.  debug=true on the url will show the sql calls
    'access callback' => 'user_access',           //Use the internal security function to test rights to the sql data
    'user callback' => 'forena_current_user_id',  //Use $user->uid when specifying :current_user in sql
);
// Uncomment the following lines and modify to connect to an external databse
// The following data would be referenced in a report as externalapp/anothersqlfilename
/*
$_forena_repositories['externalapp'] = array(
    'path' => 'data_blocks/external_sql',         //Indicate the directory that will contain the sql files.  You'll need to create this directory
    'title' => 'My APP Database',                 //Only used in error messages if there is a db connection failure
    'data provider' => 'FrxPostgres',             //Use the native postgres driver
    'uri' => "dbname=mydatabasename user=myusername password=mypassword";,  //Database connection string used to connect to the database.    
    'debug' => @$_GET['debug'],                   //This is a technique I use to help debug sql.  debug=true on the url will show the sql calls
    'access callback' => 'user_access',           //Use the internal security function to test rights to the sql data.  You can write your own function in a module and use it here. 
    'user callback' => 'forena_current_user_id',  //Use $user->uid when specifying :current_user in sql. You can write your own function in a custom module and use it here
); 
*/ 

Author
------
David Metzler