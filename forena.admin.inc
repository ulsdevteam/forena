<?php
require_once('forena.common.inc'); 
/**
 * Save the report file to disk
 *
 * @param string $name File name  to save report to
 * @param unknown_type $data
 */
function forena_save_report($name, $data) { 
  $report_path = forena_report_path();
  //@TODO: Clean up filename to make sure  
  $filepath = $report_path.'/'.$name. 'frx';
  if (is_object($data)) { 
    $data = $data->asXML(); 
  }

  try { 
    file_put_contents($filepath, $data); 
  } catch (Exception $e) {
    fornea_error('Error Saving Report', $e->getMessage()); 
  }
  
}

/**
 * Forena admin settings form
 *
 */
function forena_settings() {
  $report_path = forena_report_path();
  $path = variable_get('forena_path','reports');
   
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
  $src_dir = drupal_get_path('module','forena') . '/repos/reports';  
  if (!file_exists($path)) { 
    try { 

      if (file_exists($path)) { 
        drupal_set_message (t('Created directory %s', array($path))) ; 
      }
      mkdir($path);
      
    } catch (Exception $e) { 
      forena_error('Unable to create report directory', $e->getMessage());  
    }
   
  } elseif ($path != $src_dir) { 
    // Copy the reports from the 
    $d = dir($src_dir);
    $dest_dir = $d->path; 
    $i=0; 
    while (false !== ($rpt_file = $d->read())) {
       echo $entry."\n";
       $src_file = $d->path .'/'. $rpt_file; 
       $dest_file = $path .'/'. $rpt_file; 
       if (is_file($src_file)) {
         file_put_contents($dest_file,file_get_contents($src_file)); 
         $i++;
       } 
    }
    
    $d->close();
    drupal_set_message($i .' delivered reports copied from '. $src_dir. ' to '. $path);     
  }
  menu_cache_clear(); 
}
