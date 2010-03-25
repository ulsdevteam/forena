<?php
// $Id$
/**
 * @file
 * Report administration forms and functions. 
 */
require_once('forena.common.inc'); 
/**
 * Save the report file to disk
 *
 * @param string $name File name  to save report to
 * @param unknown_type $data
 */
function forena_save_report($data, $report, $save_file = FALSE) { 
  static $save_count=0; 
  if ($report && !is_object($report)) $report = new SimpleXMLElement($report); 
  $report_path = forena_report_path();
  //@TODO: Clean up filename to make sure  
  $name = $data['name']; 
  $filepath = $report_path .'/'. $data['name'] .'.frx';
  
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
  
  // Set default interpretations of data 
  $data['enabled'] = $data['enabled'] ? 1 : 0; 
  if (!$data['category']) $data['category'] = 'All'; 
  $data['hidden'] =  ($data['hidden'] && $data['hidden']!='N' && $data['hidden']!='0') ? 1:0; 
  
  // Save to the Database
  if (file_exists($filepath)) {
    $result = db_query("SELECT report_name FROM {forena_reports} WHERE report_name='%s'", $name); 
    if ($rpt = db_fetch_object($result)) { 
      db_query("UPDATE {forena_reports} SET title='%s', category='%s', repository='%s', access='%s'".
        ", hidden='%s', cache='%s'  WHERE report_name='%s'", 
        array($data['title'], 
          $data['category'], 
          $data['repository'], 
          $data['access'], 
          $data['hidden'], 
          $rpt_cache,
          $name )); 
    }
    else { 
      db_query("INSERT INTO {forena_reports} (report_name, title, category, repository, access, hidden, cache) ". 
        "VALUES ('%s', '%s', '%s', '%s', '%s', %d, '%s')", 
        array($name, 
          $data['title'],
          $data['category'],
          $data['repository'], 
          $data['access'],
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
  }
  $path = forena_report_path() .'/'. $subdir; 
  
  $d = dir($path); 
  if ($d) while (false !== ($rpt_file = $d->read())) {
    $src_file = $d->path .'/'. $rpt_file; 
    $dest_file = $path .'/'. $rpt_file; 
    if (is_file($src_file)) {
      list($report_name, $ext) = explode('.', $rpt_file, 2);
      if ($ext == 'frx') {  
        $report_name = trim($prefix .'/'. $report_name, '/');  
        $r_xml = new SimpleXMLElement(file_get_contents($src_file));
        // Load the report
       
        $r = new FrxReport($r_xml);  
        $data['title'] = $r->title; 
        $data['category'] = $r->category; 
        $data['repository'] = $r->repository; 
        $data['access'] = $r->access; 
        $data['name'] = $report_name; 
        $save_count = forena_save_report($data, $r_xml); 
      }
    } 
    elseif (is_dir($src_file)) {
      if (substr($rpt_file, 0, 1)!='.') { 
        $save_prefix = $prefix; 
        $prefix .=  trim('/'. $rpt_file, '/'); 
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
  $path = variable_get('forena_path', 'reports');
   
  $form['forena_path'] = array(
    '#type' => 'textfield',
    '#title' => t('URL path settings'),
    '#description' => t('Specify the url by which reports are accessed. (e.g. "reports").  Use a relative path '.
                        'and don\'t add a trailing slash or the URL won\'t work'),
    '#default_value' => $path, 
  ); 
    
  $form['forena_report_repos'] = array(
    '#type' => 'textfield',
    '#title' => t('Report Repository'),
    '#description' => t('Indicate the directory that you want to use for your reports.  In order for you to '. 
                        'to be able to save reports, this directory should be writable by the web user. Relative'. 
                        'paths should be entered relative to the base path of your drupal installation.'), 
    '#default_value' => $report_path, 
  ); 
  
  $form['instructions'] = array(
    '#type' => 'item',
    '#title' => t('Data Sources'), 
    '#value' => '<p>'. t('Database connections and data block repositories are configured directly in the file system for '.
                         'security reasons.  See the Forena Reports README.txt file for more information.') .'</p>', 
  ); 
  
  
  $form =  system_settings_form($form);
  $form['#submit'][] = 'forena_settings_submit'; 
  return $form; 
  
}
/**
 * Added submit handler to create directories and clear menu cacth
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
