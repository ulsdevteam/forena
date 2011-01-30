<?php
// $Id$
/**
 * @file
 * Common functions used throughout the project but loaded in this 
 * file to keep the module file lean. 
 */
// Include Report renderer. 
require_once('FrxReport.inc');
require_once('FrxDataProvider.inc');
  
/**
 * Get name from argument 1 or alterntaively from a file name
 *
 * @param unknown_type $name
 */
function forena_report_desc($name='') {
  if (!$name) $name = arg(1);
  // Get defined document types
  $doctypes = forena_supported_doctypes();   
  $parts = explode('.', $name);
  $name =''; 

  if ($parts) foreach ($parts as $part) { 
    if (!$doctypes[$part]) { 
      $name .= '/'. $part; 
    } 
    else { 
      $desc['format'] = $part; 
    }
  } 
  $desc['name'] = trim($name, '/');  
  $report_path = variable_get('forena_report_repos', 'reports');
  list ($dir, $name_part) = explode('/', $name_part, -2);
  if (!$name_part) $dir = ''; 
  $desc['directory'] = $report_path . '/' . $dir;    
  $desc['filename'] = $report_path .'/'. trim($name, ' /') .'.frx';
   
  if (file_exists($desc['filename'])) {
    $desc['exists'] = TRUE; 
  }
  
  return $desc; 
}

/**
 * Determines which css files need to be loaded.  
 *
 * @param array $desc Report descriptor from forena_rport_desc
 * @param string $form The report "form" to be used.  From the report
 * @param string $format Document format that will be used for the report.
 * @return array A list of css files that should be applied to the report. 
 */
function forena_report_css($desc, $form,  $format = '') {
  $css_files = array();
  // First check for the form file  
  $path = forena_report_path(); 
  if (file_exists($path . '/' . $form . '.css')) { 
    $css_files[] = $path . '/' . $form . '.css'; 
  }
  if ($format && file_exists($path .'/'. $form .'-'. $format .'.css')) {
    $css_files[] = $path . '/'. $form . '-' . $format .'.css'; 
  }
  
  // Now check for a report specific file
  $base_file = $path . '/' . $desc['name']; 
  
  if ($format && file_exists($base_file .'-' . $format .'.css')) { 
    $css_files[] = $base_file .'-'. $format . '.css';   
  }
  elseif (file_exists($base_file .'.css')) {

    $css_files[] = $base_file .'.css'; 
  }  
  //drupal_set_message("desc: <pre>". print_r($desc, 1) ."</pre>");
  //drupal_set_message("css files: <pre>". print_r($css_files, 1) ."</pre>");
  return $css_files; 
}

/**
 * Load and render a report based on a drupal path. 
 * In this function the arglist is used to get the full path to the report. 
 *
 * @return unknown
 */
function forena_report($name_in, $parms = array()) {   

  $desc = forena_report_desc(); 
  $name = $desc['name']; 
  $format = $desc['format']; 
  $filename = $desc['filename']; 
  // Determine the data to get. 
  if (!$parms) $parms = $_GET; else $parms = (array)$parms; 
  unset($parms['q']);  
  if ($name) {
    $r = forena_get_report($name, $parms);  
    if ($r) {
    //check for default parameters
    $r_params = $r->parameters;

      if ($r_params) {
        //put default parameters in parms array
        foreach ($r_params as $key => $parm) {
          if (!$parms[$key] && $parm['value']) {
            $parms[$key] = $parm['value']; 
            $reload_params = TRUE;
          }
          //do not show report if a required parameter does not have a value
          //force the user to input a parameter
          if ((!$parms[$key]) && (strcmp($parm['require'], "1") == 0) && (!$parm['value'])) {
            $missing_parms = TRUE;
          }
        }
      }
      //Reload report if parameters were missing
      if ($reload_params) {
        $r = forena_get_report($name, $parms);
      }

      // Get data from the report; 
      $r_parms = $r->parameters;       
      $form = $r->options['form'];
      $rpt_xml = $r->rpt_xml;         
      
      // Default the form
      if (!$form) $form = variable_get('forena_default_form', 'letter'); 
      $q;                
 
      //put title on top of report
      $title = $r->title; 
      if (!$missing_parms) {
        $output .= $r->render($format);    
        $css_files = forena_report_css($desc, $form, $format);
      }
               
      if ($format) { //a format was requested       
        $header = '<h1>' . $title . '</h1>';      
        $output = $header . $output;
        if ($css_files) foreach ($css_files as $css_file) { 
          $css_text .= file_get_contents($css_file); 
        }
        $options = array('css' => $css_text,
                         'docname' => str_replace(' ', '_', $title),
                         'xml' => $r_text,
                         'title' => $title[0],
                   );
        
        forena_generate_doc($format, $output, $options);
      }
      else {
        // We've got parameters so display the parameters form
        if ($r_parms) $output = drupal_get_form('forena_parameters_form') . $output; 
        //set the title        
        $title = $r->title;        
        drupal_set_title($title);
        //Creating links for downloadable documents.
        //build querystring for document href
        foreach ($parms as $key => $value) {
          $q .= "&". $key . '='. $value;
        }
        $q = trim($q, '&');
               
        //Building the document links
        $rpt_xml = $r->rpt_xml; 
        $nodes = $rpt_xml->xpath('//frx:docgen/frx:doc');        
        $div .= '<div class="doclinks">';           
        $default_doctypes = variable_get('forena_doc_formats', array()); 
        if (!$missing_parms) { 
          if (!$nodes) {            
            //show the default. All supported links
            foreach ($default_doctypes as $value) if (is_object(forena_get_doctypes($value))) {
              $div .= '<a class="doclinks" href="'. $name_in . '.'. $value .'?'. $q .'">'. strtoupper($value) .'</a>';
            }
          }
          else {            
            //There were nodes. show the prefered doc types          
            $doctypes = forena_supported_doctypes();
            foreach ($nodes as $value) {
              $arr = $value->attributes();
              $type = (string)$arr['type'];
              if ($doctypes[$type]) {                     
                if (is_object(forena_get_doctypes($type))) {
                  $div .= '<a class="doclinks" href="'. $name_in . '.'. $type .'?'. $q .'">'. strtoupper($type) .'</a>';
                }
              }                       
            }
          }
          $div .= '</div>';                
          $output = $div . '<div class="forena-report">'. $output .'</div>';
        }
        $path = drupal_get_path('module', 'forena');
        drupal_add_css($path .'/forena.css', 'module');  
        if ($css_files) foreach ($css_files as $css_file) {         	
          drupal_add_css($css_file, 'module');
        }        
        return $output;
      }   
    } 
    else { 
    	// Didn't find it so assume we've got a stale cache entry and delete it. 
    	require_once('forena.admin.inc');
    	forena_delete_report($name); 
      drupal_not_found(); 
    }
  } 
  else {
    drupal_not_found();  
  }
}


// Putting this in a function to sandbox the repository settings
function __forena_load_repository(&$repo) { 
  // First determine if the class file exisits
  $path = $repo['path']; 
  $conf = array(); 
            
  if (file_exists($path .'/settings.php')) {
    // Override with repository specific data 
    include($path .'/settings.php'); 
  }
 
  $repo = array_merge($repo, $conf);    
  // $repo['user callback'] = $conf['user callback']; 
  if (!is_object($repo['data'])) $repo['data'] = __forena_load_provider($repo, $repo['path']);  
  
  //$repo['auth'] = __forena_load_auth($security_provider); 
  
}

/**
 * Load the data provider class based on the class name. 
 *
 * @param string $name
 * @return object The data provider object
 */
function __forena_load_provider($conf, $repo_path) {
  $name = $conf['data provider'];  
  if (!$name) $name = $conf['data_engine'];
  
  forena_define_plugins(); 
  // Instantiate the path
  //drupal_set_message("conf: <pre>". print_r($conf, 1) ."</pre>  repo_path: <pre>". print_r($repo_path,1) ."</pre>");
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
    $repos['forena_help'] = array('path' => $path .'/repos/forena_help',
                             'title' => 'Forena Sample Reports');
  
    $repos['drupal'] = array('path' => $path .'/repos/drupal',
                             'title' => 'Drupal Reports');  

  }
  //drupal_set_message("repos: <pre>". print_r($repos, 1) ."</pre>");


  // Now determine if the object exists 
  if ($name) { 
    if ($repos[$name]) {

      if (!is_object($repos[$name]['data'])|| !is_object($repos[$name]['auth'])) {
        __forena_load_repository($repos[$name]); 
      }
      return $repos[$name];     
    } 
    else { 
      forena_error('Undefined repository '. $name, "Undefined Repository: $name <pre>". print_r($repos, 1) .'</pre>'); 
    }
  } 
  else { 
    return $repos; 
  }

}

/**
 * Biuld cache
 *
 * @param xhtml $r_xhtml Forena XML report.  
 * @return array data to be stored in the cache field for the report in the database. 
 */
function forena_load_cache($r_xhtml) {  
  
  $blocks = array();   
  if (is_object($r_xhtml)) {

    $block_xml = $r_xhtml->xpath('//*[@frx:block]'); 
    // Extract all the blocks and organize by provider
  
    foreach ($block_xml as $key => $block_node) {
      $attrs = $block_node->attributes('urn:FrxReports'); 
      foreach ($attrs as $key => $value) { 
        if ($key == 'block') { 
          list($provider, $block) = explode('/', $value, 2);
          $repos[$provider][] = $block;
        }          
      }
    } 
   
    if ($repos) foreach ($repos as $provider => $blocks) {       
      $repos = forena_repository($provider);      
      $provider = $repos['data'];
      $conf = $provider->conf;
      $access = array(); 

      foreach ($blocks as $block_name) {
        if ($provider && $block_name) {
          if (method_exists($provider, 'load_block')) {
            $conf = $provider->conf; 
            $block = $provider->load_block($block_name); 
            if (array_search($block['access'], $access)===FALSE) $access[]=$block['access']; 
          }
        } 
        else { 
          //drupal_set_message('no provider found', 'error');    
        }
      }
   
      if ($conf['access callback'] && $access) $cache['access'][$conf['access callback']]=$access; 
 
    }

  
  
  }
 
  return $cache; 
}

/**
 * Extract the data by running a block
 *
 * @param unknown_type $data_block
 * @param unknown_type $parameters
 * @param string $clause order / where cause
 * @return unknown
 */
function forena_invoke_data_provider($data_block, $parameters=array(), $clause='') {
   list($provider, $block) = explode('/', $data_block, 2); 
   // Get the data    
   $repos = forena_repository($provider);
   
   if ($repos['user callback']) {
     $user_fn = $repos['user callback']; 
     $parameters['current_user'] = $user_fn(); 
   }
  
   if ($repos['data']) { 
     $provider = $repos['data'];
     $access = TRUE; 
     if (method_exists($provider, 'data')) {
       $xml = $provider->data($block, $parameters, $clause); 
     }
     return $xml; 
   }
}
/**
 * Load information about a data block
 *
 * @param string $data_block provider/blockpath to use for the load. 
 * @return unknown
 */
/**
 * Returns an array of information about the data block
 * @param $data_block
 * @return unknown_type
 */
function forena_load_block($data_block, $clause='') {
	
   list($provider, $block) = explode('/', $data_block, 2); 
   // Get the data 
   
   $repos = forena_repository($provider);

   if ($repos['data']) { 
     $provider = $repos['data'];
     if (method_exists($provider, 'load_block')) {
       $block_info = $provider->load_block($block, $clause); 
       
     }
     return $block_info; 
   }
}

/**
 * Load the formatters for all initialized repositories.  
 *   
 */
function forena_get_formatter($fkey) { 
  // Get all repositories
  $repos = forena_repository();  
  
  foreach ($repos as $k => $r) {
    $provider = $r['data'];
    if ($provider && method_exists($provider, 'formats'))  {      
      $f = $provider->formats();      
    if ($f[$fkey] && method_exists($provider, $fkey)) {
        // We found an object with the advertised method return it        
        return $provider; 
      }      
    }
  }
  //Did not find the formater in the data provider
  //Look to see if it's in a control class 
  $controls = forena_define_controls();
  foreach ($controls as $k => $r) {
    $provider = $r;
    if ($provider && method_exists($provider, 'formats'))  {      
      $f = $provider->formats();      
    if ($f[$fkey] && method_exists($provider, $fkey)) {
        // We found an object with the advertised method return it        
        return $provider; 
      }      
    }
  }  
  return $formats; 
}



/**
 * Accepts the name of a file
 * 
 * Returns a xml object of the file. 
 *
*/
function forena_get_report($report_name, $data=array()) {
  if ($report_name) { 
    $report_path = forena_report_path();
    $filename = $report_path . '/'. $report_name . '.frx';    
    if (file_exists($filename)) {
      $r_text = file_get_contents($filename);
      $modified = filemtime($filepath); 
      $result = db_query("SELECT report_name, modified FROM {forena_reports} WHERE report_name='%s'", $name);
      $save = FALSE; 
      // If the file modification time has changed then save. 
      if ($rpt = db_fetch_object($result)) {
        if ($result->modified != $modified) {
        	$save = TRUE; 	       
        }
      }
      else { 
      	$save = TRUE; 
      }
      try { 
        $r = forena_report_object($r_text, $data);  
                
      } catch (Exception $e) { 
        forena_error('Unable to read report', $e->getMessage()); 
      }
      if ($save) {
      	require_once('forena.admin.inc');
      	forena_save_report($report_name,$r_text,FALSE); 
      }
    }
    return $r;
  }
}

/**
 * Object factory for forena report
 * When called without a report name, it returns the last created report.
 * This static caching mechanism is used for form functions that are called 
 * within a page load. 
 *
 * @param unknown_type $report_name
 */
function forena_report_object($report='', $data='') { 
  static $object = ''; 
  if ($report) {  	 
    $object = new FrxReport($report, $data);
  }
  return $object; 
}

/**
 * Enter description here...
 *
 * @param simplexml $xml
 * @param string $tag
 * @return string
 */
function forena_inner_xml($xml, $tag) {
  if (is_object($xml) && is_object($xml->$tag)) {
    $xml_data = $xml->$tag->asXML();
    $xml_data = preg_replace("/<\/?" . $tag . "(.|\s)*?>/", "", $xml_data);
  };
  return $xml_data; 
}

/**
 * Accepts the name of the html tag, and the string the tag is in.
 * 
 * Returns the string within the html tag name 
 * 
*/
function forena_get_html($tag, $r_text) {
  $open = strpos($r_text, $tag);
  $close = strpos($r_text, '>', $open);
  $next = strpos($r_text, '<', $close + 1);
  $str = substr($r_text, $close + 1, $next - ($close + 1));
  
  return $str;
}

function forena_format_data($value, $format, $format_str) {  
  $fo = forena_get_formatter($format);   
  if ($fo) {
    $ret = $fo->$format($value, $format_str); 
  } 
  else { 
    $ret = $value; 
  }
  return $ret; 
}

/**
 * Load a block file form disk and introspect the comments to determine security 
 * Return a structured array based on this. 
 *
 * @param unknown_type $filepath
 * @param unknown_type $comment
 */
function forena_load_block_file($filepath, $comment='--', $trim) { 
  if (!file_exists($filepath)) {
    return array();
  }
 
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
function forena_error($short_message='', $log='') { 
  if ($short_message) { 
    drupal_set_message($short_message, 'error'); 
  }
  if ($log) {
    watchdog('forena', $log, NULL, WATCHDOG_ERROR); 
  }
}

function forena_debug($short_message, $log) { 
  if ($log) {
    watchdog('forena debug', $log, NULL); 
  }
  if ($short_message) { 
    drupal_set_message($short_message); 
  }
}

/**
 * Invokes the hooks required for forena plugin registration. 
 * Each module returns an array structure that defines multiple plugins
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
function forena_define_plugins( $class='') {
  $plugins = forena_plugins(); 
  foreach ($plugins as $p) {  
    if (($class=='' || $class==$p['class'])) {       
      if ($p['file'] && $p['module']) {
        include_once( drupal_get_path('module' , $p['module']) .'/'. trim($p['file'], '/')); 
      }
    }
  }
}

/**
 * Builds a global array of available controls
 * and returns the array.
 */
function forena_controls() {
  static $controls = ''; 
  if (!$controls) {  
    $controls = array(); 
    foreach (module_list() as $module) {
      $function = $module . '_forena_controls';
      if (function_exists($function)) {
        $returned_controls = $function();
        if ($returned_controls) foreach ((array)$returned_controls as $c) { 
          $c['module'] = $module; 
          $controls[]=$c; 
        }
      }
    }
  }
  return $controls; 
}

/**
 * returns a list of instances of the control classes
 * if there wasn't a class, returns an empty string.
 */
function forena_define_controls( $class='') {
  static $instances = '';   
  if (!$instances) { 
    $classes = forena_controls();
    foreach ($classes as $c) {
      if (($class=='' || $class==$c['class'])) {
        if ($c['file'] && $c['module']) {
          include_once( drupal_get_path('module' , $c['module']) .'/'. trim($c['file'], '/'));
          if (class_exists($c['class'])) {
            $instances[$c['class']] =  new $c['class'];
          }
        }
      }
    }
  }  
  if ($class && $instances[$class]) {
    return $instances[$class];
  }
  return $instances;
}

/*
 * Recieves a datablock and returns an array of values from the data block.
 * @data_block: name of the data block to be invoked for values
 * @field: Specific field name within the data block. The values returned will only 
 * come from this field.
 * @params: filtering for the data block
 * @where_clause: Where clause for data block to be filtered against.
 * 
 */
function forena_get_data_block_params($data_block, $field, $label, $params=array(), $clause='') {
  $xml = forena_invoke_data_provider($data_block, $params, $clause);   
  
  $list = array();
  if ($xml) {     	  
    $path = ($field) ? $field : '*[1]';
    $label_path = ($label) ? $label : '*[2]';
    drupal_set_message($label);
    
    //walk through the $xml.
    $rows = $xml->xpath('*');    
    if ($rows) foreach ($rows as $row) {    	
    	$value = $row->xpath($path);
    	$label_field = $row->xpath($label_path);
    	$label_value =  $label_field ? (string)$label_field[0] : (string)$value[0]; 
    	$list[(string)$value[0]] = (string)$label_value;
    	
    }
  }
  return $list;
}

/** 
 * Form to edit parameters
 * Extra features: 
 * In the parameters section of the report are new attributes
 * frx:parm: 
 * @data_source = data block for the parameter to values from
 * @data_field = specific field of the data block to recieve values from.
 * if no field is specified then the first column is arbitrarily chosen.
 * @type = The form element that will display the values: select, radios are supported
 * default is textfield.
 * 
 * This function will evaluate each parameter, determine its type, data_source, and data_field
 * and display those values appropriately.
 * 
 */
function forena_parameters_form($form_state) {
  $parms = $_GET;
  unset($parms['q']); 
  
  $r = forena_report_object(); 
  
  if ($r) {
      drupal_set_title($r->title);
      $head = $r->rpt_xml->head;
      
      $form = array();
      $form['params'] = array(
        '#tree' => TRUE, 
        '#title' => 'Parameters', 
        '#type' => 'fieldset', 
        '#collapsible' => TRUE, 
        '#collapsed' => $r->blocks_loaded,
      ); 
      
      $nodes = $head->xpath('frx:parameters/frx:parm');
      
      foreach ($nodes as $node) {
      	$label = (string)$node['label'];   
        $id = (string)$node['id'];
        $data_source = (string)$node['data_source'];
        $data_field = (string)$node['data_field'];
        $type = (string)$node['type'];
        $value = ($parms[$id]) ? $parms[$id] : (string)$node['value'];  
        $desc =  (string)$node['desc'];
        $label_field = (string)$node['label_field']; 
        
        (strcmp((string)$node['require'], "1") == 0) ? $required = TRUE : $required = FALSE; 
                
        //Determine the form element type to be displayed
        //If select or radios is chosen then begin a $list array for display values.
        switch ($type) {
        	case 'select':
        		$list = array('--select--');
        		break;
        	case 'radios';
        	 $list = array('--choose--');
        	 break;
        	default:
        	 $type = 'textfield';
        	 $list = '';        	 	       	 
        }
        
        //If a data_source attr was found then create an array of
        //returned values filtered against data_field attr.
      	if ($data_source) {
      		$list = forena_get_data_block_params($data_source, $data_field, $label_field);      		
      	}		
      		
        $form['params'][$id] = array (
      		'#type' => $type,
      		'#title' => ($label) ? t($label) : t($id),      		
          '#default_value' => $value,
          '#required' => $required,
          '#description' => $desc,
        );
        
        //if $list is not empty then push options 
        //onto the array. options will cause an error for 
        //textfield elements.       
        if ($type != 'textfield') {
        	$form['params'][$id]['#options'] = $list;
        }      	
      }      

      $form['params']['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Submit',
      );
    }

  return $form;
}

/**
 * 
 * gets the values from the params form
 * redirects to the report page with the values in 
 * the querystring.
 */
function forena_parameters_form_submit($form, &$form_state) {
  $values = $form_state['values'];   
  foreach ($values['params'] as $key => $value) {
    $q .= '&'. $key .'='. urlencode($value);
  }
  $q = trim($q, '&'); 
  $form_state['redirect']= array($_GET['q'], $q); 
}

/**
 * Gets the correct format function
 * to render the document and returns an 
 * object of that class.
 * 
 * If it fails it returns a 0;
 */

function forena_get_doctypes($fkey) {
  $controls = forena_define_controls();
  foreach ($controls as $k => $r) {
    $provider = $r;
    if ($provider && method_exists($provider, 'doc_types'))  { 
      $f = $provider->doc_types();      
      if ($f[$fkey] && method_exists($provider, $f[$fkey])) {
        // We found an object with the advertised method return it        
        return $provider; 
      }      
    }
  }  
  return 0;
}

/**
 * 
 * @return returns an array of supported document format extensions
 * 
 */
function forena_supported_doctypes() {
  $controls = forena_define_controls();
  $supported_doctypes = array();
  foreach ($controls as $k => $r) {
    $provider = $r;
    if ($provider && method_exists($provider, 'doc_types'))  { 
      $f = $provider->doc_types(); 
      $supported_doctypes = array_merge($supported_doctypes, $f);
    }
  }
  $temp = array_keys($supported_doctypes);
  $supported_doctypes = array_combine($temp, $temp);  
  return $supported_doctypes;
}

/**
 * Returns an object of the template class 
 * that has a method named templates.
 * 
 * If it fails it returns a 0;
 */
function forena_get_templates($fkey) {
  $controls = forena_define_controls();
  foreach ($controls as $k => $r) {
    $provider = $r;
    if ($provider && method_exists($provider, 'templates'))  { 
      $f = $provider->templates();            
      if ($f[$fkey] && method_exists($provider, $fkey)) {
        // We found an object with the advertised method, return it        
        return $provider; 
      }      
    }
  }  
  return 0;
}

/**
 * 
 * @return returns an array of supported templates
 * 
 */
function forena_supported_templates() {
  $controls = forena_define_controls();
  $supported_templates = array();
  foreach ($controls as $k => $r) {
    $provider = $r;
    if ($provider && method_exists($provider, 'templates'))  { 
      $f = $provider->templates(); 
      $supported_templates = array_merge($supported_templates, $f);
    }
  }   
  return $supported_templates;
}

function forena_supported_formats() {
  $controls = forena_define_controls();
  $supported_formats = array();
  $f = array();
  foreach ($controls as $k => $r) {
    $provider = $r;
    if ($provider && method_exists($provider, 'formats'))  { 
      $f = $provider->formats(); 
      $supported_formats = array_merge($supported_formats, $f);
    }
  }   
  return $supported_formats;
}

/**
 * 
 * @param unknown_type $format: The extension of the document to be rendered
 * @param unknown_type $output: The string of the page to be displayed
 * @return: The document in the requested format. Returns a string if not
 * able to format.
 */
function  forena_generate_doc($format, $output, $options = array()) {

  $doc = forena_get_doctypes($format);
  if ($doc) {
    $all_methods = $doc->doc_types();
    $method = $all_methods[$format];
    $ret = $doc->$method($output, $options);
  }
  if ($ret) {
  // If an object was found, set the appropriate mime type (header doctype). 
 //   $flen = strlen($ret);
    switch ($format) { 
      case 'doc':
        header('Content-Type: application/msword');
        header('Cache-Control:');
        header('Pragma:');
        header("Cache-Control: must-revalidate");
        break; 
      case 'pdf': 
        header('Content-Type: application/pdf');
        header('Cache-Control:');
        header('Pragma:');
        header("Cache-Control: must-revalidate");
        header("Content-Disposition: attachment; filename=report.pdf");
        break;
   
      case 'xls':
        header('Content-Type: application/msexcel');
        header('Cache-Control:');
        header('Pragma:');
        header("Cache-Control: must-revalidate");
        header("Content-Disposition: attachment; filename=report.xls");  
        break;   
     case 'csv':
        header('Content-Type: application/csv');
        header('Cache-Control:');
        header('Pragma:');
        header("Cache-Control: must-revalidate");
        header("Content-Disposition: attachment; filename=report.csv");     
        break;

    }
    // Print the output. 	
    print $ret; 
  }
  else {
    print $output;
  }   
}  

function forena_get_user_reports() { 
  $result = db_query('SELECT * FROM {forena_reports} where hidden=0 ORDER BY category,title asc');
  $reports = array();
  while ($row = db_fetch_object($result)) {
    $access = TRUE; 
    $cache = $row->cache; 
     
    if ($cache) {            
      $cache = unserialize($cache);
      
      // Check each callback function to see if we have an error. 
      if ($cache['access']) foreach ($cache['access'] as $callback => $args) {
        if ($callback) foreach ($args as $arg) {
          $access = FALSE;            
          if (function_exists($callback)) { 
            $a = $callback($arg); 
          }
          if ($a) $access = TRUE;           
        } 
        else {          
          $access = TRUE; 
        }
      }
    }    
    if ($access) {      
      $reports[$row->category][] = array(
        'title' => $row->title, 
        'report_name' => $row->report_name, 
      );
    }   
  }
  return $reports; 
}

/**
 * Render the my reports category block
 *
 */
function forena_my_reports_block() { 
  $reports = forena_get_user_reports();
  if (!$reports) return ''; 
  $output = '<ul>'; 
  foreach ($reports as $category=>$reports) {
    $output .= '<li>'. l($category, 'forena', array(fragment => urlencode($category))). '</li>';  
  }
  $output .= '</ul>';
  return $output; 
}