<?php
// Include Report renderer. 
require_once('FrxReport.inc');
require_once('FrxDataEngine.inc');
  
// Putting this in a function to sandbox the repository settings
function __forena_load_repository(&$repo) { 
  // First determine if the class file exisits
  $path = $repo['path']; 
  if (file_exists($path.'/settings.php')) {
    // This should at minimum set  
    include($path. '/settings.php'); 
     
  }
  
  $repo['data'] = __forena_load_engine($conf, $repo['path']); 
  //$repo['auth'] = __forena_load_auth($security_provider); 
  
}

/**
 * Load the data engine class based on the class name. 
 *
 * @param string $name
 * @return object The data engine object
 */
function __forena_load_engine($conf, $repo_path) { 
  $name = $conf['data_engine']; 
  $path = drupal_get_path( 'module', 'forena'). '/plugins/' . $name; 
  // Make sure that we don't override predefined classes
  if (!class_exists($name) && file_exists($path. '.inc')) { 
    include_once($path.'.inc'); 
  }
  if (class_exists($name)) { 
    $o = new $name($conf, $repo_path); 
    return $o; 
  }
}

/*
 * Get the repository data for an a repository name.  
 * If no repository is specified the descriptors for all repositories are returned. 
 */
function forena_repository($name='') { 
  global $forena_repositories; 
  static $repos =''; 

  
  // Empty repository so we need to initialize
  if (!$repos) {   
    // Build the default sample one
    $path = drupal_get_path('module','forena');
    
    $repos = array();
    // Load the repository list from the global settings.php file.  
    if ($forena_repositories) { 
      $repos = $forena_repositories; 
    }
  
    // Overide difinitions of the sample and drupal repositories. 
    $repos['sample'] = array('path' => $path.'/repos/sample',
                             'title' => 'Forena Sample Reports');
  
    $repos['drupal'] = array('path' => $path.'/repos/drupal',
                             'title' => 'Drupal Reports');
  }
  
  // Now determine if the object exists 
  if ($name) { 
    if ($repos[$name]) {
      if (!is_object($repos[$name]['data'])|| !is_object($repos[$name]['auth'])) {
        __forena_load_repository($repos[$name]); 
      }
      return $repos[$name];     
    }
  } else { 
    return $repos; 
  }
    
}


function forena_invoke_data_engine($data_block, $parameters=array(), $subquery='') {
   list($provider,$block) = explode('/',$data_block, 2); 
   // Get the data 
   $repos = forena_repository($provider); 
   if ($repos['data']) { 
     $engine = $repos['data'];
     if (method_exists($engine,'data')) {
       $xml = $engine->data($block, $parameters, $subquery); 
     }
     return $xml; 
   }
}

/**
 * Load a block file form disk and introspect the comments to determine security 
 * Return a structured array based on this. 
 *
 * @param unknown_type $filepath
 * @param unknown_type $comment
 */
function forena_load_block_file($filepath, $comment='--') { 
  $block_data = file_get_contents($filepath); 
  $lines = explode("\n",$block_data); 
  $cnt = count($lines);
  $access = '';
  $i=0;  
  $block = ''; 
  while ($i<$cnt) { 
    $l = trim($lines[$i],"\r"); 
    list($d,$c) = explode($comment,$l,2);
    if  ($c) { 
      list($a,$o) = explode('=', $c, 2);
      if (trim($a)=='ACCESS') { 
        $access = trim($o);    
      }
    } 
    if (strpos($l,$comment)!==0) { 
      $data .= "$l\n";   
    }
    $i++;
  }
  return array('access'=>$access, 'source' => $data); 
  
}

function forena_error($short_message, $log) { 
  if ($short_message) { 
    drupal_set_message($short_message,'error'); 
  }
  if ($log) {
    watchdog('forena',$log, NULL, WATCHDOG_ERROR); 
  }
}


