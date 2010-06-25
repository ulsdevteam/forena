<?php
// $Id$
/**
 * @file
 * Report administration forms and functions. 
 */
require_once('forena.common.inc'); 

function _forena_verify_directory($fullpath, $recursive=FALSE) { 
  static $path=''; 
  if (!$recursive) $path = forena_report_path();
  list($dir, $file) = explode('/', $fullpath, 2); 
  $path .= '/'. $dir; 
  // Path 
  if (!file_exists($path)) {
    mkdir($path); 
  }
  // Recurse to next file. 
  if ($file && strpos($file, '/')) {
    watchdog('debug', $file); 
    _forena_verify_directory($file, TRUE); 
  }
}

/**
 * Save the report file to disk
 *
 * @param string $name File name  to save report to
 * @param unknown_type $data
 */
function forena_save_report($report_name, $report, $save_file = FALSE) { 
  static $save_count=0;  
  if ($report && !is_object($report)) {
    try {
      $report = new SimpleXMLElement($report);
    } 
    catch (Exception $e) {
      forena_error(t('Could not save %s because XML was malformed', htmlspecialchars($report_name)), 
        "<p>Invalid XML</p><pre>XML:". htmlspecialchars($report) ."\n". $e->getMessage() ."</pre>"); 
      return; 
    }
  }
  _forena_verify_directory($report_name); 
  $report_path = forena_report_path();
  $r = new FrxReport($report);    
  $data['title'] = $r->title; 
  $data['category'] = $r->category; 
  $data['options'] = $r->options; 
  $data['name'] = $report_name; 
  //@TODO: Clean up filename to make sure  
  $name = $data['name']; 
  $filepath = $report_path .'/'. $report_name .'.frx';
  // If we need to save this to the file system 
  if ($save_file) {
    // Serialize the report for saving
    if (is_object($report)) { 
      $r_xml = $report->asXML(); 
    }
    
    try { 
      file_put_contents($filepath, $r_xml); 
    } catch (Exception $e) {
      fornea_error('Error Saving Report', $e->getMessage()); 
    }
  }
 
  // Get the security caches from the reports  
  if ($report) $cache = forena_load_cache($report); else $cache=''; 

  if ($cache) $rpt_cache = serialize($cache); 
  //drupal_set_message('saving'.$name. print_r($cache,1)); 
  // Set default interpretations of data 
  $data['enabled'] = $data['enabled'] ? 1 : 0; 
  if ((string)$data['options']['hidden']) {
    $data['hidden'] =  ($data['options']['hidden'] && $data['options']['hidden']!='N' && $data['options']['hidden']!='0') ? 1:0;
    if (!$data['category']) $data['category'] = 'All'; 
  } 
  else {
    // Set hidden based on category
    $data['hidden'] = ($data['category'])? 0 : 1;  
  }
  
  // Save to the Database
  if (file_exists($filepath)) {
 
    $result = db_query("SELECT report_name FROM {forena_reports} WHERE report_name='%s'", $name); 
    if ($rpt = db_fetch_object($result)) {
      db_query("UPDATE {forena_reports} SET title='%s', category='%s'".", hidden='%s', cache='%s'  WHERE report_name='%s'", 
        array($data['title'], 
          $data['category'], 
          $data['hidden'], 
          $rpt_cache,
          $name )); 
    }
    else { 

      db_query("INSERT INTO {forena_reports} (report_name, title, category, hidden, cache) ". 
        "VALUES ('%s', '%s', '%s',  %d, '%s')", 
        array($name, 
          $data['title'],
          $data['category'],
          $data['hidden'],
          $rpt_cache,
          )); 
    }
    $save_count++; 
    
  }  
  return $save_count;
}

/**
 * Syncronize the data 
 *
 */
function forena_db_sync($subdir='') {
  static $prefix = '';
  if (!$subdir) { 
    $prefix = ''; 
     db_query('delete from {forena_reports}'); 
  }
  $path =forena_report_path() .'/'. $subdir; 
  $d = dir($path); 
  if ($d) while (false !== ($rpt_file = $d->read())) {
    $src_file = trim($d->path, '/') .'/'. trim($rpt_file, '/'); 
    $dest_file = $path .'/'. trim($rpt_file, '/'); 
    if (is_file($src_file)) {
      list($report_name, $ext) = explode('.', $rpt_file, 2);
      if ($ext == 'frx') {  
        $report_name = trim($prefix .'/'. $report_name, '/');

        try { 
          $r_xml = new SimpleXMLElement(file_get_contents($src_file));
        } catch (Exception $e) { 
          $s = t('Invalid Report %s', $src_file); 
          forena_error($s, $s . $e->getMessage()); 
          
        }
        
        // Load the report
       
        $r = new FrxReport($r_xml);  


        $save_count = forena_save_report($report_name, $r_xml); 
      }
    } 
    elseif (is_dir($src_file)) {
      if (strpos($rpt_file, '.')!==0) { 
        $save_prefix = $prefix; 
        $prefix .=  '/'. $rpt_file; 
        $prefix = trim($prefix, '/'); 
        forena_db_sync($prefix); 
        $prefix = $save_prefix; 
      }
    }
  }    
  if ($d) $d->close(); 
  return $save_count; 
}


/**
 * Forena admin settings form
 *
 */
function forena_settings() {
  $report_path = forena_report_path();


  $form['forena_report_repos'] = array(
    '#type' => 'textfield',
    '#title' => t('Report Repository'),
    '#description' => t('Indicate the directory that you want to use for your reports.  In order for you to '. 
                        'to be able to save reports, this directory should be writable by the web user. Relative'. 
                        'paths should be entered relative to the base path of your drupal installation.'), 
    '#default_value' => $report_path, 
  ); 
  
  $form['forena_input_format'] = filter_form(variable_get('forena_input_format', FILTER_FORMAT_DEFAULT), null, array('forena_input_format'));
  
  $form['instructions'] = array(
    '#type' => 'item',
    '#title' => t('Data Sources'), 
    '#value' => '<p>'. t('Database connections and data block repositories are configured directly in the file system for '.
                         'security reasons.  See the Forena Reports README.txt file for more information.') .'</p>', 
  ); 
  
  $form['forena_default_form'] = array(
    '#type' => 'textield', 
    '#title' => t('Default report form'), 
    '#description' => t('Indicates the default style sheet(s) that will be used when no tag is present in the report.'), 
  ); 
  
  
  $form =  system_settings_form($form);
  $form['#submit'][] = 'forena_settings_submit'; 
  return $form; 
  
}
/**
 * Added submit handler to create directories and clear menu cache
 *
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_settings_submit($form, &$form_state) { 
  $values = $form_state['values']; 
  $path = $values['forena_report_repos'];
  $src_dir = drupal_get_path('module', 'forena') . '/repos/reports';  
  if (!file_exists($path)) { 
    try { 

      if (file_exists($path)) { 
        drupal_set_message(t('Created directory %s', array($path))) ; 
      }
      mkdir($path);
      
    } catch (Exception $e) { 
      forena_error(t('Unable to create report directory'), $e->getMessage());  
    }
   
  } 
  
  if (file_exists($path) && $path != $src_dir) { 
    // Copy the reports from the 
    $d = dir($src_dir);
    $dest_dir = $d->path; 
    $i=0; 
    while (false !== ($rpt_file = $d->read())) {
       echo $entry ."\n";
       $src_file = $d->path .'/'. $rpt_file; 
       $dest_file = $path .'/'. $rpt_file; 
       if (is_file($src_file)) {
         file_put_contents($dest_file, file_get_contents($src_file)); 
         $i++;
       } 
    }
    
    $d->close();
    drupal_set_message($i .' delivered reports copied from '. $src_dir .' to '. $path);
  }
  $save_count = forena_db_sync();
  drupal_set_message('Imported '. $save_count .' forms into the database');       
  menu_cache_clear(); 
}




/**
 * Form function for the edit report form
 * @param $form_state
 * @return the form
 */
function forena_edit_form($form_state) {
  $desc = forena_report_desc();
  $name = $desc['name']; 
  $filename = $desc['filename']; 
  $format = $desc['format']; 
  if ($name) {
    if ($desc['exists']) {
      $r = forena_get_report($name);
      drupal_set_title($r->title);
      $form = array();
      $title = (string)$r->title;
      
      // Get the attributes to save them away
      if ($r) $attributes = $r->get_attributes_by_id(); 
      
      
      $category = (string)$r->category;
      $body = $r->body->asXML();
      //array of xml attributes that are required to have a value
      $required = array('id' => TRUE, 'label' => TRUE);
      //list of supported document formats
      $supported = forena_supported_doctypes();
      $doclist = variable_get('forena_doc_formats', array());          
      
      $form['report_name'] = array(
        '#type' => 'hidden',
        '#value' => $name,      
      );
      
      $form['head'] = array(
        '#type' => 'value',
        '#value' => $r->rpt_xml->head,
      );
      
      $form['attributes'] = array(
        '#type' => 'value', 
        '#value' => $attributes,
      ); 
      
      $form['save_report_name'] = array(
        '#type' => 'textfield',
        '#title' => t('Report Name'),
        '#default_value' => $name,
        '#description' => t('Enter only letters, numbers, and special characters: <.>, <->, <_>. White space is not permitted.'),
      );
      $form['title'] = array(
        '#type' => 'textfield',
        '#title' => t('Title'),
        '#default_value' => $title,
      );

      $form['category'] = array(
        '#type' => 'textfield',
        '#title' => t('Category'),
        '#default_value' => $category,
      );

      $form['params'] = array('#tree' => TRUE, '#type' => 'fieldset', '#title' => 'Params', '#collapsible' => TRUE, '#collapsed' => TRUE );
      $nodes = $r->rpt_xml->head->xpath('frx:parameters/frx:parm');
      if ($nodes) foreach ($nodes as $node) {
        //make a subtree of params
        $form['params'][(string)$node['id']] = array('#tree' => TRUE, '#type' => 'fieldset', '#title' => (string)$node['label'], '#collapsible' => TRUE, '#collapsed' => TRUE );

        //now walk the attributes of each node
        //and make a subtree for each node
        $w_count = 0; // increments the weight, so form elements show in order	
        foreach ($node->attributes() as $key => $value) {
          if (strcmp($key, 'require') == 0) {
            $form['params'][(string)$node['id']][(string)$key] = array(
             '#type' => 'radios',
             '#title' => $key,
             '#default_value' => $value,
             '#options' => array("1" => t('Yes'), "0" => t('No')),
            );
          }
          else{
              $form['params'][(string)$node['id']][(string)$key] = array(
                '#type' => 'textfield',
                '#title' => $key,
                '#default_value' => $value,
                '#required' => $required[$key],
                '#weight'  => ($required[$key]) ? -20 + $w_count : 0,
              );
          }
          $w_count ++;
        }

        //add the default value
        $form['params'][(string)$node['id']]['default'] = array(
          '#type' => 'textfield',
          '#title' => 'Default Value',
          '#default_value' => (string)$node,
          '#required' => FALSE,
        );
      }
      
      //begin checking doc generation options
      $nodes = $r->rpt_xml->head->xpath('frx:docgen/frx:doc');      
      
      $form['docgen'] = array('#tree' => TRUE, '#type' => 'fieldset', '#title' => 'Document Options', '#collapsible' => TRUE, '#collapsed' => TRUE );
      //build the options and default list
      $options = array();
      $default = array();             
      foreach ($doclist as $key => $value) {
        if (is_object(forena_get_doctypes($value))) {
          $options[$value] = strtoupper($value); 
          $doc =  $r->rpt_xml->head->xpath('frx:docgen/frx:doc[@type="' . $value . '"]');  
          if ($doc && $doclist[$value]) {
            $default[$value] = $value;
          }
        }
      } 
      //display checkboxes
      $form['docgen']['docs'] = array(
        '#type' => 'checkboxes',          
        '#description' => (t('*If no options are selected, the system will display all of the above as available for this report.')),
        '#options' => $options,
        '#default_value' => $default
      );
      

      $form['body'] = array(
        '#type' => 'textarea',
        '#title' => t('Body'),
        '#default_value' => $body,
        '#rows' => 25,
      );
      
      $form['format'] = filter_form(variable_get('forena_input_format', FILTER_FORMAT_DEFAULT));
      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Save',
      );
      return $form;
    }
    else {
      drupal_not_found();
    }
  }
  else {
    drupal_not_found();
  }  
}

function forena_edit_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  $regexp = "/^[A-Za-z0-9\/\.\_\-]*$/";
  $save_report_name = $values['save_report_name'];
  $report_name = $values['report_name'];
  
  //checking illegal characters
  if (!preg_match($regexp, $save_report_name)) {
    form_set_error('save_report_name', t('Invalid character entered in Report Name'));
  }
  
  //comparing the report names to see if they have changed.
  //If they have, making sure the new name does not already exist.
  if ($report_name != $save_report_name) {
    $report_path = forena_report_path();
    $filename = $report_path .'/'. $save_report_name .'.frx'; 
    if (file_exists($filename)) {
      form_set_error('save_report_name', t('The file ' . $save_report_name . ' already exists. Please enter another name.'));
    }
  }
}
/**
 * builds a string of the xml document,
 * submits it to forena_save_report.
 */
function forena_edit_form_submit($form, &$form_state) {  
  $values = $form_state['values'];
  $h = $values['head'];
  //$xml = '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">'. "\n"; 
  $xml = '<html xmlns:frx="urn:FrxReports">'. "\n"; 
  $xml .= '  <head>'. "\n";
  $xml .= '    <title>'. htmlspecialchars($values['title']) .'</title>'. "\n";
  $xml .= '    <frx:category>'. htmlspecialchars($values['category']) .'</frx:category>'. "\n"; 

  $nodes = $h->xpath('frx:options');
  if ($nodes) $xml .= '    ' . $nodes[0]->asXML() . "\n";

  $xml .= '    <frx:parameters>'. "\n";
  $default_value;
  if ($values['params']) foreach ($values['params'] as $key => $value) {
    $name = $key;
    $xml .= '      <frx:parm ';
    foreach ($values['params'][$key] as $n => $v) {
      //capture default value, concatenate it outside of tag.
      if (! (strcmp($n, 'default') == 0)) {
        $xml .= htmlspecialchars($n) . '="' . htmlspecialchars($v) . '" ';
      }
      else{
        $default_value = htmlspecialchars($v);
      }
    }
    $xml .= '>' . $default_value . '</frx:parm>' . "\n";
  }
  $xml .= '    </frx:parameters>' . "\n";
  
  //check if docgen is avaialable for report
  if ($values['docgen']) {
    $xml .= '    <frx:docgen>' . "\n";
    //if no options were checked, default to select all of them
    if ($selected = array_filter($values['docgen']['docs'])) {
      foreach ($selected as $key => $value) {
      $xml .= '      <frx:doc type="'. $value . '"/>'."\n";
      }
    }
    $xml .= '    </frx:docgen>' . "\n";
  }
  
  $nodes = $h->xpath('frx:fields');
  if ($nodes) $xml .= $nodes[0]->asXML();
  $body = $values['body'];
  // Put the body tag back on if it was stripped.   

  if (strpos($body, '<body')===FALSE) {
    $body = forena_clean_xhtml('<body>'. $body .'</body>'); 
  }
  
  // Build the html
  $xml .= "\n" . '  </head>' . "\n";    
  $xml .= $body; 
  $xml .= '</html>'; 
  
  $file = $values['save_report_name'];
  $o = forena_report_object($xml);
  // If there are no frx attributes in the body then replace them with the old values.
  $frx_nodes = $o->rpt_xml->xpath('body//*[@frx:block]'); 
  if (!$frx_nodes) {
    drupal_set_message('saving attributes'); 
    $o->save_attributes_by_id($values['attributes']);
  } 
  if (forena_save_report($file, $o->rpt_xml, TRUE) == 1) {
    drupal_set_message('Your report, "' . $file . '" has been saved.');
   // $form_state['redirect']= 'reports/' . str_replace('/','.',$file);
  } 
}

/*
 * administer the settings for document format options
 */
function forena_doc_formats_settings() {
  $supported_doctypes = forena_supported_doctypes();
    
  $form['forena_doc_formats'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Document Formats'),
    '#default_value' => variable_get('forena_doc_formats', $supported_doctypes),
    '#description' => t('check your desired document format'),
    '#options' => $supported_doctypes,
  );
  return system_settings_form($form);
}

/**
 * 
 * @param $form_state
 * @return a form to edit the fields of the report
 */
function forena_fields_form($form_state) {
  $desc = forena_report_desc();
  $name = $desc['name']; 
  $filename = $desc['filename']; 
  $format = $desc['format']; 
  if ($name) {
    if (file_exists($filename)) {
      $r = forena_get_report($name);
      drupal_set_title($r->title);
      $regexp = '/\{[^}]+}/';
      $match = array("{", "}");

      /*search the body, looking for {}*/
      $body = $r->body->asXML();  
      $head = $r->rpt_xml->head;
         
      $fields = array();    
      preg_match_all($regexp, $body, $out);
      foreach ($out as $key => $value) {
        foreach ($value as $el) {
          $clean_element = str_replace($match, "", $el);                
          $fields[$clean_element] = $clean_element;          
        }
      }
      
      $form = array();
      $form['head'] = array(
        '#type' => 'hidden',
        '#value' => $head,      
      );
      
      $form['body'] = array(
        '#type' => 'hidden',
        '#value' => $r->body,      
      );
      
      $form['report_name'] = array(
        '#type' => 'hidden',
        '#value' => $name,
      );
      
      $form['fields'] = array('#tree' => TRUE, '#type' => 'fieldset', '#title' => 'Fields', '#collapsible' => FALSE, '#collapsed' => FALSE );
      /*Now check the fields in the body against the xml*/
      foreach ($fields as $field) {
        $form['fields'][$field] = array('#tree' => TRUE, 
          '#type' => 'fieldset',
          '#title' => $field,
          '#collapsible' => FALSE, 
          '#collapsed' => FALSE, );
        $path = 'frx:fields/frx:field[@id="' . $field . '"]'; 
        $node =  $head->xpath($path);
        $attr = $node[0];
        
        $form['fields'][$field]['format'] = array(
          '#type' => 'textfield',
          '#title' => t('format'),
          '#default_value' => $attr['format'],
          '#size' => 30,                  
        );
            
        $form['fields'][$field]['format-string'] = array(
          '#type' => 'textfield',
          '#title' => t('format-string'),
          '#default_value' => $attr['format-string'],
          '#size' => 30,                  
        );
            
        $form['fields'][$field]['link'] = array(
          '#type' => 'textfield',
          '#title' => t('link'), 
          '#default_value' => $attr['link'],
          '#size' => 30,                   
        ); 
        
        $form['fields'][$field]['default'] = array(
          '#type' => 'textfield',
          '#title' => t('default value'), 
          '#default_value' => $attr,
          '#size' => 30,                   
        ); 
        
      }    
      $form['fields']['submit'] = array(
        '#type' => 'submit',
        '#value' => 'Save',
      );
      return $form;
    }
    else {
      drupal_not_found();
    }
  }
  else {
    drupal_not_found();
  }
}


function forena_fields_form_submit($form, &$form_state) {  
  $values = $form_state['values'];
  $head = $values['head'];
  $body = $values['body'];
  $fields = $values['fields'];  

  
  $xml = '<html xmlns:frx="urn:FrxReports">'. "\n";
  $xml .= '  <head>'. "\n";
  
  /*walk through all the children of the head and build the xml*/
  foreach ($head->children() as $node) {
    $xml .= '    ' . $node[0]->asXML() . "\n";
  }
  foreach ($head->children(FRX_NS) as $frx_node) {
    $name = (string)$frx_node->getName();
    if (strcmp($name, 'fields') != 0) {
      $xml .= '    ' . $frx_node[0]->asXML() . "\n";
    }
  }
  
  /*now build the fields*/
  $xml .= '    <frx:fields>' . "\n";
  foreach ($fields as $key => $value) {
    //skip the submit key  
    if(strcmp($key, 'submit') != 0) {
    	//skip fields that have no attributes
	    if (! ($value['format'] == '' && $value['format-string'] == '' && $value['link'] == '' && $value['default'] == '')) {
	    	$xml .= '      <frx:field id="' . $key . '" ';
        if ($value) foreach ($value as $k => $v) {
		      if (strcmp($k, 'default') != 0 && $value[$k]) {
		    	  $xml .= ' ' . $k . '="' . $v . '"'; 
		    	}   	
		    }
		    /*if there is not a default value, self close the tag.*/
		    if ($value['default']) {
		    	$xml .= '>' . $value['default'] . '</frx:field>' . "\n";
		    }
		    else{
		    	$xml .= '/>' . "\n";
		    }	    
	    }
    }   
  }
  $xml .= '    </frx:fields>' . "\n";
  $xml .= '  </head>' . "\n";
  $xml .= $body->asXML();
  $xml .= '</html>';
  
  //drupal_set_message("xml: " . htmlspecialchars($xml));
  $file = $values['report_name'];

  if (forena_save_report($file, $xml, TRUE) == 1) {
    drupal_set_message('Your report, "' . $file . '" has been saved.');
    $form_state['redirect']= 'reports/' . $file;
  }
  else {
    drupal_set_message('There was an error saving your report, "'. $file . '" to the database');
  }
}

/**
 * A form to preview and add data blocks to an existing report
 * @param unknown_type $form_state
 * @return unknown_type
 */
function forena_add_block_form(&$form_state) {
	$desc = forena_report_desc();
  $name = $desc['name']; 
  $filename = $desc['filename']; 
  $format = $desc['format']; 
  if ($desc['exists']) { 
    $r = forena_get_report($name);
    drupal_set_title($r->title);
    $form = array();
    $title = (string)$r->title;

    $template_array = forena_supported_templates(); 		
		
		$form = array();
		
		$form['report_name'] = array(
        '#type' => 'value',
        '#value' => $name,      
      );    
    
      
		$from['report'] = array(
		  '#type' => 'value',
		  '#value' => $r,
		); 
    
    //find the datablocks in the existing report
    $data_block_array = array();
    $r->get_attributes_by_id(); 
    $body = $r->body;    
    $path = '//*[@frx:block]';
   
    foreach ($body->xpath($path) as $node) {
    	 $attrs = $node->attributes(FRX_NS); 
    	 $id = (string)$node['id']; 
       $data_block_array[$id] = (string)$attrs['block'];
    }
    
    $form['delete_blocks'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Data Blocks In Report'),
      '#options' => $data_block_array,
      '#description' => t('Check the data block to be deleted from your report.'),    
    );
		
    $form['delete'] = array(
      '#type' => 'submit',
      '#value' => 'Delete',      
      '#submit' => array('forena_delete_block_report'),     
    );
     
    
		
		$form['data_block'] = array(
		  '#type' => 'textfield',
	    '#title' => t('Data Block'),
		  '#default_value' => $form_state['storage']['data_block'],		  
	    '#description' => t('Enter a data block, a preview will appear below'),	
		);
		
		$form['templates'] = array(
			'#type' => 'radios',
			'#title' => t('Templates'),
			'#default_value' => ($form_state['storage']['templates']) ? $form_state['storage']['templates'] : $template_array['table'],
		  '#options' => $template_array,
		  '#required' => TRUE,
		);
		
		$form['output'] = array(    
		  '#type' => 'markup',
		  '#value' => '</p>'. $form_state['storage']['output'] .'</p>',	  
		);
		
		$form['preview'] = array(
	    '#type' => 'submit',
		  '#value' => 'Preview',
		);
		
		if (isset($form_state['storage']['output'])){
			$form['add'] = array(
        '#type' => 'submit',
        '#value' => 'Add',
			  '#submit' => array('forena_add_block_report'),
      );
		}
		
    return $form;
  }
  else {    	
    drupal_not_found();
  }    
}

/**
 * Validates the forena_add_block_form's datablock field.
 * @param $form
 * @param $form_state
 * @return unknown_type
 */

function forena_add_block_form_validate($form, &$form_state) {
	$values = $form_state['values'];
	$data_block = $values['data_block'];
	$delete_blocks = $values['delete_blocks'];
	$templates = $values['templates'];	

	//validate the data block only if delete block was not chosen
	if(!$delete_blocks) {
		$block_info = forena_load_block($data_block);	
		if(!$block_info) {
			form_set_error('data_block', t('Invalid data block.'));
	  }
	}
}



/**
 * The Preview submit handler for forena_add_block_form
 * Renders datablock into a report
 * @param $form
 * @param $form_state
 * @return unknown_type
 */
function forena_add_block_form_submit($form, &$form_state) {
	$values = $form_state['values'];
	$data_block = $values['data_block'];
	$template = $values['templates'];
	
	$form_state['storage']['data_block'] = $data_block;
	if ($data_block)  {
		//get xml from data block
		$xml = forena_invoke_data_engine($data_block);
	}
	if ($xml) { 	
		//create an array of columns
		$rows = $xml->xpath('//row');
		$column_array = array();
			
		foreach ($rows as $columns) {
			foreach ($columns as $name => $value) {
				$column_array[$name] = $name;
			}
		}
	
		//create xml from template
		$template_obj = forena_get_templates($template);	
		if($template_obj) {
	    $body = $template_obj->$template($column_array, $data_block);
		}
		
		$rpt_xml =  '<html xmlns:frx="urn:FrxReports"><body>'. $body. '</body></html>';    
		
		//render the xml
		$output = forena_render_report($rpt_xml);
	

	  $form_state['storage']['output'] = $output;
	  $form_state['storage']['template'] = $body;  
	} 	
}

/**
 * Add submit handler for forena_add_block_form. 
 * This adds the datablock to an existing report.
 *
 */
function forena_add_block_report($form, &$form_state) {
  $values = $form_state['values'];  
  $report_name = $values['report_name'];
  $data_block = $values['data_block'];
  $template = $values['templates'];
  $added_report = $form_state['storage']['template']; 
  $r = forena_get_report($report_name);  
  $head = (string)$r->rpt_xml->head->asXML();
  $main_report = forena_inner_xml($r->rpt_xml,'body'); 
  $new_report =  '<html xmlns:frx="urn:FrxReports">'. $head. '<body>'. $main_report . $added_report. '</body></html>';  
  
  if (forena_save_report($report_name, $new_report, TRUE) == 1) {
    drupal_set_message('Your report, "' . $file . '" has been saved.');
   // $form_state['redirect']= 'reports/' . str_replace('/','.',$file);
  } 
}

/**
 * Delete submit handler to delete report blocks
 * @param $form
 * @param $form_state
 * @return unknown_type
 */
function forena_delete_block_report($form, &$form_state) {
	$values = $form_state['values'];
	$delete_blocks = array_filter($values['delete_blocks']);
		
	$report_name = $values['report_name'];
	$r = forena_get_report($report_name);

	
  //find the datablocks in the existing report
  
  $r->get_attributes_by_id(); 
  $body = $r->body;   
  
  foreach ($delete_blocks as $id) {  	
  	$r->deleteNode($id);  	
  }
  
 if (forena_save_report($report_name, $r->rpt_xml, TRUE) == 1) {
    drupal_set_message(t('Your report has been saved.'));
   // $form_state['redirect']= 'reports/' . str_replace('/','.',$file);
  } 
	
}

/**
 * Clean xhtml 
 *
 * @param unknown_type $xhtml
 * @return unknown
 */
function forena_clean_xhtml($xhtml) { 
  $ret = $xhtml; 
  // If tidy is installed lets clean the html using that. 
  if (is_callable('tidy_repair_string')) { 
    $config = array(
      'doctype'            => 'omit', 
      'indent'             => 2,
      'input-xml'          => true, 
      'output-xml'         => true, 
      'indent-attributes'  => false,
      'indent-spaces'      => 2,
      'wrap'               => 135 );
    $ret = tidy_repair_string($xhtml, $config, 'utf8');
  }    
  else { 
    $ret = str_replace('&nbsp;', '&#160;', $ret);
  }
  return $ret; 
}









