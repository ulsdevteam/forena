<?php
// $Id$
/**
 * @file
 * Common functions used throughout the project but loaded in this 
 * file to keep the module file lean. 
 */
// Include Report renderer. 
require_once('FrxReport.inc');
require_once('FrxDataEngine.inc');
  
// Putting this in a function to sandbox the repository settings
function __forena_load_repository(&$repo) { 
  // First determine if the class file exisits
  $path = $repo['path']; 
  if (file_exists($path .'/settings.php')) {
    // This should at minimum set  
    include($path .'/settings.php'); 
     
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

  forena_define_plugins('data'); 
  // Instantiate the path
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
  global $_forena_repositories; 
  static $repos =''; 

  
  // Empty repository so we need to initialize
  if (!$repos) {   
    // Build the default sample one
    $path = drupal_get_path('module', 'forena');
    
    $repos = array();
    // Load the repository list from the global settings.php file.  
    if ($_forena_repositories) { 
      $repos = $_forena_repositories; 
    }
  
    // Overide difinitions of the sample and drupal repositories. 
    $repos['sample'] = array('path' => $path .'/repos/sample',
                             'title' => 'Forena Sample Reports');
  
    $repos['drupal'] = array('path' => $path .'/repos/drupal',
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
  } 
  else { 
    return $repos; 
  }
    
}

function forena_load_cache($r_xhtml) {
  $blocks = array(); 
  if (is_object($r_xhtml)) {
    $block_xml = $r_xhtml->xpath('//*[@frx:block]'); 
    // Extract all the blocks and organize by provider
    foreach ($block_xml as $key => $block_node) {
      $attrs = $block_node->attributes('urn:FrxReports'); 
      foreach ($attrs as $key => $value) { 
        list($provider, $block) = explode('/', $value, 2);
        $repos[$provider][] = $block;          
      }
    } 
    
    foreach ($repos as $provider => $blocks) { 
      $repos = forena_repository($provider);
      $engine = $repos['data'];
      $conf = $engine->conf;
      $access = array(); 
      foreach ($blocks as $block_name) {
        if ($engine && $block_name) {
          if (method_exists($engine, 'load_block')) {
            $conf = $engine->conf; 
            $block = $engine->load_block($block_name); 
            if (array_search($block['access'], $access)===FALSE) $access[]=$block['access']; 
          }
        } 
        else { 
          //drupal_set_message('no engine found', 'error');    
        }
      }
      $cache['access'][$conf['access callback']]=$access; 
    } 
  }
  return $cache; 
}

/**
 * Extract the data by running a block
 *
 * @param unknown_type $data_block
 * @param unknown_type $parameters
 * @param unknown_type $subquery
 * @return unknown
 */
function forena_invoke_data_engine($data_block, $parameters=array(), $subquery='') {
   list($provider, $block) = explode('/', $data_block, 2); 
   // Get the data 
   $repos = forena_repository($provider);

   if ($repos['data']) { 
     $engine = $repos['data'];
     if (method_exists($engine, 'data')) {

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
function forena_load_block_file($filepath, $comment='--', $trim) { 
  $block_data = file_get_contents($filepath); 
  $lines = explode("\n", $block_data); 
  $cnt = count($lines);
  $access = '';
  $i=0;  
  $block = ''; 
  while ($i<$cnt) { 
    $l = trim($lines[$i], "\r"); 
    list($d, $c) = explode($comment, $l, 2);
    if ($trim) $c = trim($d, $trim); 
    if  ($c) { 
      list($a, $o) = explode('=', $c, 2);
      if (trim($a)=='ACCESS') { 
        $access = trim($o);    
      }
    } 
    if (strpos($l, $comment)!==0) { 
      $data .= "$l\n";   
    }
    $i++;
  }
  return array('access' => $access, 'source' => $data); 
  
}

/**
 * General wrapper procedure for reporting erros
 *
 * @param string $short_message Message that will be displayed to the users
 * @param string $log Message that will be recorded in the logs. 
 */
function forena_error($short_message, $log) { 
  if ($short_message) { 
    drupal_set_message($short_message, 'error'); 
  }
  if ($log) {
    watchdog('forena', $log, NULL, WATCHDOG_ERROR); 
  }
}

/**
 * Load the report repository path
 *
 * @return unknown
 */
function forena_report_path() { 
  $report_path = variable_get('forena_report_repos', ''); 
  if (!$report_path) {
    $report_path = drupal_get_path('module', 'forena') . '/repos/reports';
  }
  return $report_path; 
}

/**
 * Invokes the hooks required for forena plugin registration. 
 * Each module returns an array structure that defines multiple plugins
 * 
 * types - Type of object: 
 *         data  - Provides the data methods 
 *         control - implements a rendering control
 *         formatter - Implments a data formatter that is called when processing token replacements
 * 
 * class - Indicates the name of the class that will be used to define the plugins behaviors. 
 * 
 */
function forena_plugins() {
  static $plugins = ''; 
  if (!$plugins) {  
    $plugins = array(); 
    foreach (module_list() as $module) {
      $function = $module .'_forena_plugins';
      if (function_exists($function)) {
        $returned_plugins = $function();
        if ($returned_plugins) foreach ((array)$returned_plugins as $p) { 
          $p['module'] = $module; 
          $plugins[]=$p; 
        }
      }
    }
  }
  return $plugins; 
}

/**
 * Loads all of the include files that 
 */
function forena_define_plugins($type ='', $class='') {
  $plugins = forena_plugins(); 
  foreach ($plugins as $p) { 
    if ((!$type || $type==$p['type']) && ($class='' || $class=$p['class'])) { 
      if ($p['file'] && $p['module']) { 
        include_once( drupal_get_path('module' , $p['module']) .'/'. trim($p['file'], '/')); 
      }
    }
  }
}

