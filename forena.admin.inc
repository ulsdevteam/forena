<?php
/**
 * @file
 * Report administration forms and functions.
 */
require_once('forena.common.inc');


/**
 * Display reports to edit for admins in the structure menu
 * Enter description here ...
 */
function forena_admin_reports() {
  GLOBAL $language;
  $links[] = array('href' => 'reports/add', 'title' => 'Create New Report' );
  $output = theme_links($links, array('class' => 'action-links'));
  $headers = array(t('title'), t('name'), t('category'), t('operation'));
  $result = db_query("SELECT * FROM {forena_reports} where language='%s' ORDER BY category,title", $language->language);
  while ($row = db_fetch_object($result)) {
    $rpt = str_replace('/', '.', $row->report_name);
    $edit = l('Edit', 'reports/' . $rpt . '/edit');
    $clone = l('Clone', 'reports/add/' . $rpt);
    $delete = l('Delete', 'reports/' . $rpt . 'delete');
    $title = l($row->title, 'reports/' . $rpt);
    $data[] = array($title, $row->report_name, $row->category, $edit . ' ' . $clone . ' '. $delete);
  }
  $output .= theme_table($headers, $data);
  return $output;
}

/**
 * Make sure a drectory exists in the report path prior to save.
 *
 * @param unknown_type $fullpath
 * @param unknown_type $recursive
 */
function _forena_verify_directory($fullpath, $recursive=FALSE) {
  static $path='';
  $success = TRUE;
  if (!$recursive) {
	  $path = forena_report_path();
	  if (!is_writable($path)) {
  	  drupal_set_message(t('Report directory %s is not modifiable', array('%s' => $path)), 'error');
  	  return FALSE;
	  }
  }
  @list($dir, $file) = explode('/', $fullpath, 2);
  $path .= '/' . $dir;

  // Path
  if (!file_exists($path) && $file) {
    @mkdir($path);
    if (!@is_writable($path)) {
    	drupal_set_message(t('Error creating directory %path', array('%path' => $path)), 'error');
    	return FALSE;
    }

  }
  // Recurse to next file.
  if ($file && strpos($file, '/')) {
    _forena_verify_directory($file, TRUE);
  }
  return TRUE;
}

/**
 * Recursively ,  all report files from the source directory to the destination directory
 *
 * @param string $src_dir  Source directory
 */
function _forena_copy_reports($src_dir, $overwrite=FALSE, $subdir=FALSE) {
  static $dest_dir = '';
  static $i = 0;
  if (!$subdir) {
    $dest_dir = forena_report_path();
  }

  if (!file_exists($dest_dir)) {
    mkdir($dest_dir);
  }

  $d = dir($src_dir);
  while (false !== ($rpt_file = $d->read())) {
    $src_file = $d->path . '/' . $rpt_file;
    $dest_file = $dest_dir . '/' . $rpt_file;

    if (is_file($src_file)) {
      if ($overwrite || !file_exists($dest_file)) {
        file_put_contents($dest_file, file_get_contents($src_file));
        $i++;
      }
    }
    else {
      if (strpos($rpt_file, '.')!==0) {
        // Recurse into sub directory
        $src_save = $src_dir;
        $dest_save = $dest_dir;
        $dest_dir .= '/' . $rpt_file;
        $src_dir .= '/' . $rpt_file;
        _forena_copy_reports($src_dir, $overwrite, TRUE);
        $src_dir = $src_save;
        $dest_dir = $dest_save;
      }
    }
  }
  $d->close();
  return $i;
}

/**
 * Save the report file to disk
 *
 * @param string $name File name  to save report to
 * @param unknown_type $data
 */
function forena_save_report($report_name, $report, $save_file = FALSE, $altered = 0) {
  static $save_count=0;
  if ($report && !is_object($report)) {
    try {
      $report = new SimpleXMLElement($report);
    }
    catch (Exception $e) {
      forena_error(t('Could not save %s because XML was malformed', htmlspecialchars($report_name)),
        "<p>Invalid XML</p><pre>XML:" . htmlspecialchars($report) . "\n" . $e->getMessage() . "</pre>");
      return;
    }
  }

  if (!_forena_verify_directory($report_name)) {
    drupal_set_message(t('Error creating directory.  Check Permissions'), 'error');
    return 0;
  };
  $report_path = forena_report_path();
  $r = new FrxReport($report);
  $data['title'] = $r->title;
  $data['category'] = $r->category;
  $data['options'] = $r->options;
  $data['descriptor'] = $r->descriptor;
  $data['name'] = $report_name;
  $data['altered'] = $altered;
  $r_xml = $report->asXML();
  //@TODO: Clean up filename to make sure
  $name = $data['name'];
  $filepath = $report_path . '/' . $report_name . '.frx';
  // If we need to save this to the file system

  if ($save_file) {
    // Serialize the report for saving

    try {
      file_put_contents($filepath, $r_xml);
    } catch (Exception $e) {
      fornea_error('Error Saving Report', $e->getMessage());
    }
  }

  // Get the security caches from the reports
  if ($report) $cache = forena_load_cache($report); else $cache='';
  $rpt_cache='';
  if ($cache) $rpt_cache = serialize($cache);

  // Set default interpretations of data
  $data['enabled'] = (isset($data['enabled']) && $data['enabled']) ? 1 : 0;
  if (isset($data['options']['hidden']) && (string)$data['options']['hidden']) {
    $data['hidden'] =  ($data['options']['hidden'] && $data['options']['hidden']!='N' && $data['options']['hidden']!='0') ? 1:0;
    if (!$data['category']) $data['category'] = 'All';
  }
  else {
    // Set hidden based on category
    $data['hidden'] = ($data['category'])? 0 : 1;
  }

  // Save language info
  $lang = 'en';
  if (module_exists('locale')) {
    @list($tlang, $tname) = explode('/', $name,2);
    if (array_key_exists($tlang, language_list())) {
      $lang = $tlang;
      $name = $tname;
    }
  }

  // Save to the Database
  if (file_exists($filepath)) {
    $modified = filemtime($filepath);
    $result = db_query("SELECT report_name FROM {forena_reports} WHERE report_name='%s'", $name);

    if ($rpt = db_fetch_object($result)) {
      db_query("UPDATE {forena_reports}
      SET title='%s',
        descriptor='%s',
        category='%s',
        hidden='%s',
        cache='%s',
        src='%s',
        altered = %d,
        modified=%d  WHERE report_name='%s' AND language='%s'",
        array($data['title'],
          $data['descriptor'],
          $data['category'],
          $data['hidden'],
          $rpt_cache,
          $r_xml,
          $data['altered'],
          $modified,
          $name,
          $lang ));

    }
    else {
      db_query("INSERT INTO {forena_reports} (report_name, language, title, descriptor, category,
         hidden, cache, src, altered,  modified) ".
        "VALUES ('%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', %d, %d)",
          $name,
          $lang,
          $data['title'],
          $data['descriptor'],
          $data['category'],
          $data['hidden'],
          $rpt_cache,
          $r_xml,
          $data['altered'],
          $modified
          );
    }

    $save_count++;
  }

  $r=null;
  $result = null;
  $r_xml = null;
  $report = NULL;
  $data = NULL;
  $rpt = null;
  $cache = null;
  $rpt_cache = null;
  // Destroy object to clear memory leaks.
  if ($r) {
  	$r->__destruct();
  	unset($r);
  }


  return $save_count;
}

/**
 * Remove the report from the database and file system.
 * @param string $report_name
 */
function forena_delete_report($report_name, $delete_file = TRUE) {
  $report_path = forena_report_path();
  $language = 'en';


  $filepath = $report_path .'/'. $report_name .'.frx';
  $info = pathinfo($filepath);
  $do = TRUE;
  if (file_exists($filepath)) {
    chdir($info['dirname']);
    if ($delete_file) $do = unlink($info['basename']);
  }
  if (module_exists('locale')) {
    @list($tlang, $tname) = explode('/', $report_name, 2);
    if (array_key_exists($tlang, language_list())) {
      $report_name = $tname;
      $language = $tlang;
    }
  }
  if ($do) {
    DB_QUERY("DELETE FROM {forena_reports} WHERE report_name='%s' AND language='%s'", $report_name, $language);
  }
  else {
    drupal_set_message('Unable to delete file '. $info['basename'],'error');
  }
}

/**
 * Syncronize the data
 *
 */
function forena_db_sync($subdir='') {
  static $prefix = '';
  if (!$subdir) {
    $prefix = '';
     db_query('DELETE FROM {forena_reports}');
  }
  $save_count=0;
  $path =forena_report_path() . '/' . $subdir;
  $d = dir($path);
  if ($d) while (false !== ($rpt_file = $d->read())) {
    $src_file = trim($d->path, '/') . '/' . trim($rpt_file, '/');
    $dest_file = $path . '/' . trim($rpt_file, '/');
    if (is_file($src_file)) {
      list($report_name, $ext) = explode('.', $rpt_file, 2);
      if ($ext == 'frx') {

        $report_name = trim($prefix . '/' . $report_name, '/');

        try {
          $r_xml =file_get_contents($src_file);
        } catch (Exception $e) {
          $s = t('unable to load Report %s', $r_xml);
          forena_error($s, $s . $e->getMessage());

        }

        // Load the report

        $r = new FrxReport($r_xml);
        $save_count++;
        $save_count = forena_save_report($report_name, $r_xml, FALSE);
        $r->__destruct();
        unset($r);

      }
    }
    elseif (is_dir($src_file)) {
      if (strpos($rpt_file, '.')!==0) {
        $save_prefix = $prefix;
        $prefix .=  '/' . $rpt_file;
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
 * Accepts the name of a file
 *
 * Returns an editor object of the file.
 *
*/
function forena_get_report_editor($report_name) {
  require_once('FrxReportEditor.inc');
  if ($report_name) {
    $r_text='';
    $report_path = forena_report_path();
    $filename = $report_path . '/' . $report_name . '.frx';
    if (file_exists($filename)) {
      $r_text = file_get_contents($filename);
    }
    $r = new FrxReportEditor($r_text);
    return $r;
  }
  else {
    return new FrxReportEditor();
  }
}

/**
 * Forena admin settings form
 *
 */
function forena_settings() {

  $report_path = forena_report_path();

  $form['sync_overwrite'] = array(
    '#type' => 'checkbox',
    '#title' => t('Revert all delivered reports to orignial'),
    '#description' => t('All customizations to module delivered reports will be lost.')
  );

  $form['sync'] = array(
    '#type' => 'submit',
    '#value' => t('Clear Cache'),
    '#submit' => array('forena_settings_sync_submit'),
  );

  $form['forena_report_repos'] = array(
    '#type' => 'textfield',
    '#title' => t('Report Repository'),
    '#description' => t('Indicate the directory that you want to use for your reports.  In order for you to ' .
                        'to be able to save reports, this directory should be writable by the web user. Relative' .
                        'paths should be entered relative to the base path of your drupal installation.'),
    '#default_value' => $report_path,
  );

  $form['forena_last_report_path'] = array(
    '#type' => 'value',
    '#value' => forena_report_path(),
  );


  $form['forena_input_format'] = filter_form(variable_get('forena_input_format', FILTER_FORMAT_DEFAULT), NULL, array('forena_input_format'));

  $form['forena_default_form'] = array(
    '#type' => 'textfield',
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
  if ($path != $values['forena_last_report_path']) {
    if (!file_exists($path)) {
      try {

        if (file_exists($path)) {
          drupal_set_message(t('Created directory %s', array($path))) ;
        }
        mkdir($path);

      } catch (Exception $e) {
        forena_error(t('Unable to create report directory'), $e->getMessage());
        return;
      }
    }
  }
  forena_sync_reports();
}

function forena_sync_reports($overwrite=FALSE) {
  $directories = module_invoke_all('forena_report_directory');
  $i=0;
  if ($directories) {
    foreach ($directories as $src_dir) {
      $i += _forena_copy_reports($src_dir, $overwrite);
    }
    if ($i > 0) drupal_set_message($i . ' reports copied');
  }

  forena_db_sync();

}

function forena_forena_report_directory() {
    return drupal_get_path('module', 'forena') . '/repos/reports';
}


function forena_settings_sync_submit($form, &$form_state) {
  forena_sync_reports($form_state['values']['sync_overwrite']);
  drupal_set_message(t('Report cache cleared'));
}


function forena_format_form($form_state, $report_name) {
  $desc = forena_report_desc($report_name);
  $name = $desc['name'];
  if (!$desc['exists']) {
  	drupal_not_found();
  	exit;
  }
  $filename = $desc['filename'];
  $format = isset($desc['format']) ? $desc['format'] : '';
  if ($desc['exists']) {
    $r = forena_get_report_editor($name);
    drupal_set_title($r->title);
    $form = array();
    $r = forena_get_report_editor($name);
    $frx_options = $r->getOptions();
    $report_form = @$frx_options['form'];
    $supported = FrxReportGenerator::instance()->supported_doctypes();
    $doclist = variable_get('forena_doc_formats', array());

      $form['report_name'] = array(
        '#type' => 'value',
        '#value' => $name,
      );

     $form['form'] = array(
        '#type' => 'textfield',
        '#title' => t('Form'),
        '#default_value' => $report_form,
        '#description' => t('The page style of your report.  Specify the base filename of any css or js. ')
      );

            //begin checking doc generation options
      if ($r) $nodes = $r->simplexml->head->xpath('frx:docgen/frx:doc');
      if ($doclist) {
        $form['docgen'] = array(
         '#tree' => TRUE,
         '#type' => 'fieldset',
         '#title' => t('Document Options'),
         '#description' => t('These are document transformation options. Options selected will display as links in your report view.')
        );

        //build the options and default list
        $options = array();
        $default = array();
        foreach ($doclist as $key => $value) {
          if (is_object(FrxReportGenerator::instance()->get_doctypes($value))) {
            $options[$value] = strtoupper($value);
            $doc =  isset($r) ? $r->simplexml->head->xpath('frx:docgen/frx:doc[@type="' . $value . '"]') : '';
            if ($doc && $doclist[$value]) {
              $default[$value] = $value;
            }
          }
        }

        //display checkboxes
        $form['docgen']['docs'] = array(
          '#type' => 'checkboxes',
          '#description' => t('If no options are selected, the system will display all of the above as available for this report.'),
          '#options' => $options,
          '#default_value' => $default
        );

        $form['submit'] = array(
          '#type' => 'submit',
          '#value' => 'Save',
        );
      }
  }
  return $form;
}

function forena_format_form_submit($form, &$form_state) {
   $values = $form_state['values'];
  $name = $values['report_name'];

  $r = forena_get_report_editor($name);
  $options = array(
    'form' => $values['form']);
  $r->setOptions($options);


  // Doc gen settings.
  if (isset($form['docgen'])) {
    $docgen = array();
    if($selected = array_filter(@$values['docgen']['docs'])) {
      if ($selected) foreach ($selected as $key => $value) {
       if ($value) $docgen[] = array('type' => $key);
      }
    }
    $r->setDocgen($docgen);
  }

  if (forena_save_report($name, $r->asXML(), TRUE) == 1) {
    drupal_set_message(t('Your report, %s has been saved.', array('%s' => $name)));
  }
  else {
    drupal_set_message(t('There was an error saving your report, %s to the database.', array('%s' => $name)));
  }

}


/**
 * Form function for the edit report form
 * @param $form_state
 * @return the form
 */
function forena_layout_form($form_state, $report_name) {
	$name_in = $report_name;
  $desc = forena_report_desc($report_name);

  $name = $desc['name'];
  global $language;
  //determine if this is an add new report request

  $filename = $desc['filename'];
  $format = @$desc['format'];
  if ($name) {
    if ((isset($desc['exists']) && $desc['exists'])) {
      $save_name = $name;
      //set the name to empty string for new reports
      $r = forena_get_report_editor($name);

      $title = (string)$r->title;
      if (module_exists('locale')) {
        @list($tlang,  $tsave_name) = explode('/', $name, 2);
        // FInd out if the starting name of the report is an installed language.
        if (array_key_exists($tlang, language_list() )) {
          $lang = $tlang;
          $save_name = $tsave_name;
        }
        else {
          $lang = 'en';
        }
      }
      drupal_set_title(filter_xss($r->title));
      // Need to get all option attributes
      $frx_options = $r->getOptions();
      $hidden = @$frx_options['hidden']=='1' ? 1 : 0;
      $report_form = @$frx_options['form'];
      $attributes = $r->get_attributes_by_id();
      $category = $r->getCategory();
      $body = $r->simplexml->body->asXML();

      $form = array();

      //array of xml attributes that are required to have a value
      $required = array('id' => TRUE, 'label' => TRUE);
      //list of supported document formats
      $supported = FrxReportGenerator::instance()->supported_doctypes();
      $doclist = variable_get('forena_doc_formats', array());

      $form['report_name'] = array(
        '#type' => 'value',
        '#value' => $name,
      );

      $form['name_in'] = array(
        '#type' => 'value',
        '#value' => $name_in,
      );

      $form['attributes'] = array(
        '#type' => 'value',
        '#value' => $attributes,
      );

      $form['title'] = array(
        '#type' => 'textfield',
        '#title' => t('Title'),
        '#default_value' => $title,
      );

      $form['body'] = array(
        '#type' => 'textarea',
        '#title' => t('Body'),
        '#default_value' => $body,
        '#rows' => 25,
      );

     $form['format'] = filter_form(variable_get('forena_input_format', FILTER_FORMAT_DEFAULT));

      $form['visibility'] = array(
        '#type' => 'fieldset',
        '#title' => 'Visibility'
      );

      $form['visibility']['category'] = array(
        '#type' => 'textfield',
        '#title' => t('Category'),
        '#default_value' => $category,
        '#autocomplete_path' => 'forena/categories/autocomplete',
        '#description' => t('The heading your report will be grouped under on the report list.'),
      );

      $form['visibility']['hidden'] = array(
        '#type' => 'checkbox',
        '#title' => t('Hidden'),
        '#default_value' => $hidden,
        '#description' => t('Hide your report from showing up on the report list.'),
      );

      $form['buttons']['save'] = array(
        '#type' => 'submit',
        '#value' => 'Save',
        '#submit' => array('forena_layout_form_submit'),
      );

      if (user_access('delete report')) {
        $form['buttons']['delete'] = array(
          '#type' => 'submit',
          '#value' => 'Delete',
          '#submit' => array('forena_edit_delete_submit'),
        );
      }
      return $form;
    }
    else {
      drupal_not_found();
      exit;
    }
  }
  else {
    drupal_not_found();
    exit;
  }
}



/**
 * builds a string of the xml document,
 * submits it to forena_save_report.
 */
function forena_layout_form_submit($form, &$form_state) {
  $nodes = array();
  $values = $form_state['values'];
  $report_name = $values['report_name'];
  $r = forena_get_report_editor($report_name);
  // Title and category
  $r->setTitle($values['title']);
  $r->setCategory($values['category']);
  // Form options
  $options = array(
    'hidden' => $values['hidden'],
  );
  $r->setOptions($options);

  // Body

  $r->setBody($values['body']);
  // If there are no frx attributes in the body then replace them with the old values.
  $frx_nodes = $r->simplexml->xpath('body//*[@frx:block]');
  if (!$frx_nodes) {
    $r->save_attributes_by_id($values['attributes']);
  }

  //determine redirection.
  $report_path = forena_report_path();
  $filename = $report_path . '/' . $report_name . '.frx';

  if (forena_save_report($report_name, $r->asXML(), TRUE) == 1) {
    drupal_set_message(t('Your report, %s has been saved.', array('%s'=>$report_name)));
  }
}


function forena_add_report_form($form_state, $report_name='') {
  $name = '';
  $filename = '';
  $format = '';
  if ($report_name) {
    $desc = forena_report_desc($report_name);
    $name = $desc['name'];
    $filename = $desc['filename'];
    $format = @$desc['format'];
  }
  $form = array();
  global $language;
  //determine if this is an add new report request

  $r = forena_get_report_editor($name);
  $title = (string)$r->title;
  if (module_exists('locale')) {
    @list($tlang,  $tsave_name) = explode('/', $name, 2);
    // FInd out if the starting name of the report is an installed language.
    if (array_key_exists($tlang, language_list() )) {
      $lang = $tlang;
      $save_name = $tsave_name;
    }
    else {
     $lang = 'en';
    }
  }
  // Need to get all option attributes
  $frx_options = $r->getOptions();
  $hidden = @$frx_options['hidden']=='1' ? 1 : 0;
  $report_form = @$frx_options['form'];
  $attributes = $r->get_attributes_by_id();
  $category = $r->getCategory();
  $body = $r->simplexml->body->asXML();


  //array of xml attributes that are required to have a value
  $required = array('id' => TRUE, 'label' => TRUE);
  //list of supported document formats
  $supported = FrxReportGenerator::instance()->supported_doctypes();
  $doclist = variable_get('forena_doc_formats', array());

  $form['save_report_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Report Name'),
    '#description' => t('Enter only letters, numbers, and special characters:  - _ /
                         <br/>White space is not permitted.
                         Create a directory using the format: (directory name) / (report name). Save multiple reports to the same directory
                         by referencing the same name.'),
    '#required' => TRUE,
  );


  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $title,
    '#required' => true,
  );

  $form['category'] = array(
    '#type' => 'textfield',
    '#title' => t('Category'),
    '#default_value' => $category,
    '#autocomplete_path' => 'forena/categories/autocomplete',
    '#description' => t('The heading your report will be grouped under on the report list.'),
  );

  $form['hidden'] = array(
    '#type' => 'checkbox',
    '#title' => t('Hidden'),
    '#default_value' => $hidden,
    '#description' => t('Hide your report from showing up on the report list.'),
  );

  $form['report_name'] = array(
    '#title' => t('Create from report'),
    '#type' => 'textfield',
    '#autocomplete_path' => 'forena/reports/autocomplete',
    '#default_value' => $name,
  );


  $form['save'] = array(
    '#type' => 'submit',
    '#value' => 'Create',
  );
  return $form;

}

function forena_add_report_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  $regexp = "/^[A-Za-z0-9\/\_\-]*$/";
  $save_report_name = $values['save_report_name'];
  $desc = forena_report_desc($save_report_name);

  //comparing the report names to see if they have changed.
  //If they have, making sure the new name does not already exist.
    $report_path = forena_report_path();
    $filename = $report_path . '/' . $save_report_name . '.frx';
    if (@$desc['exists']) {
      form_set_error('save_report_name', t('The file ' . $save_report_name . ' already exists. Please enter another name.'));
    }
}

function forena_add_report_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $report_name = $values['save_report_name'];
  $desc = forena_report_desc($report_name);
  $report_name = $desc['name'];

  $r = forena_get_report_editor($values['report_name']);
  // Title and category
  $r->setTitle($values['title']);
  $r->setCategory($values['category']);
  // Form options
  $options = array(
    'hidden' => $values['hidden'],
  );
  $r->setOptions($options);

  //determine redirection.
  $filename = $desc['filename'];

  if (forena_save_report($report_name, $r->asXML(), TRUE) == 1) {
    drupal_set_message(t('Your report, %s has been saved.', array('%s'=>$report_name)));
    //if this is a new report then redirect to data blocks
    if ($values['report_name']) {
    	$edit = '/edit';
    }
    else {
    	$edit = '/edit/data/add';
    }
    $form_state['redirect']= 'reports/' . str_replace('/', '.', $desc['base_name']) . $edit;
    unset($form_state['storage']);

  }
}

/**
 * Handle delete buttons from edit forms.
 * @return unknown_type
 */
function forena_edit_delete_submit($form, &$form_state) {
	$link = 'reports/' . $form_state['values']['name_in'] ;
  $destination = '';
  if (isset($_REQUEST['destination'])) {
    $destination = drupal_get_destination();
    unset($_REQUEST['destination']);
  }
  unset($form_state['storage']);
  $form_state['redirect'] = array('path' => $link . '/delete', 'query' => array('destination' => $destination));
}


function forena_create_trans_form(&$form_state, $report_name) {
  $name = '';
  $filename = '';
  $format = '';
  $desc = forena_report_desc($report_name);
  $name = $desc['name'];
  $filename = $desc['filename'];
  $base_name = $desc['base_name'];
  $format = @$desc['format'];
  $form = array();
  global $language;
  $languages = language_list();
  //determine if this is an add new report request
  $r = forena_get_report_editor($name);
  $title = (string)$r->title;
  $lang = @$_GET['new_language'];

  $form['base_name'] = array(
    '#type' => 'value',
    '#value' => $base_name
  );
  $form['report_name'] = array(
    '#type' => 'value',
    '#value' => $name,
  );

  foreach ($languages as $key=>$obj) {
    $languages[$key] = $obj->native;
  }
  $form['report_lang'] = array(
    '#type' => 'value',
    '#value' => $lang,
  );
  $def_lang = $lang ? $lang : 'en';
  $form['save_report_language'] = array(
    '#type' => 'select',
    '#title' => t('Language'),
    '#options' => $languages,
    '#default_value' => $def_lang,
  );


  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#default_value' => $title,
    '#required' => true,
  );


  $form['save'] = array(
    '#type' => 'submit',
    '#value' => 'Create',
  );
  return $form;

}

function forena_create_trans_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $base_name = $values['base_name'];
  $desc = forena_report_desc(str_replace('/', '.' , $values['report_name']));
  $filename = $desc['filename'];
  $report_name = $desc['name'];
  $r = forena_get_report_editor($report_name);



  $new_name = $values['save_report_language']. '.' . $base_name;
  $desc = forena_report_desc($new_name);
  $filename = $desc['filename'];
  $report_name = $desc['name'];
;

  // Title and category
  $r->setTitle($values['title']);

  //determine redirection.

  if (file_exists($filename)) {
    drupal_set_message(t('Report %s already exists', array('%s' => $new_name)), 'error');
    return;
  }

  if (forena_save_report($report_name, $r->asXML(), TRUE) == 1) {
    drupal_set_message(t('Translation,  %s has been created. Switch languages to translate  ', array('%s'=>$values['title'])));
    //if this is a new report then redirect to data blocks
    $form_state['redirect']= array( 'reports/' . $values['save_report_language'] . '.' . str_replace('/', '.', $base_name) . '/edit/layout');
    unset($form_state['storage']);

  }
}


/*
 * administer the settings for document format options
 */
function forena_doc_formats_settings() {
  $supported_doctypes = FrxReportGenerator::instance()->supported_doctypes();
  $form['forena_doc_formats'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Allowed Document Formats'),
    '#default_value' => variable_get('forena_doc_formats', $supported_doctypes),
    '#description' => t('check your desired document format'),
    '#options' => $supported_doctypes,
  );

  $form['forena_doc_defaults'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Default Document Formats'),
    '#default_value' => variable_get('forena_doc_defaults', array()),
    '#description' => t('check your desired document format'),
    '#options' => $supported_doctypes,
  );

  $form['forena_email_override'] = array(
    '#type' => 'checkbox',
    '#title' => 'Run email merges in test mode' ,
    '#default_value' => variable_get('forena_email_override',FALSE),
    '#description' => t('When this box is checked emails are sent to the currently logged in user.  Useful for testing environments.'),
  );

  return system_settings_form($form);
}


/*
 * administer the settings for document format options
 */
function forena_data_settings() {
  $repos = FrxReportGenerator::instance()->app->repositories();
  $r_list = array();
  $headers = array(t('Name'),t('Description'), t('Path'), t('Operation'));
  foreach ($repos as $name=>$r) {
    $r_list[] = array(
      $name,
      $r['title'],
      $r['path'],
      l('configure', 'admin/settings/forena/data/configure/'. $name)
    );

  }

  $output = '<ul class="action-links"><li>' . l('Add data source', 'admin/settings/forena/data/add') . '</li></ul>';
  $output .= theme_table( $headers,  $r_list);
  return $output;
}


function forena_data_settings_edit(&$form_state, $source=-1) {
  $adding = ($source === -1);
  if (!@$form_state['storage']) {
    if ($adding) {
      $form_state['storage'] = array(
        'name' => '',
        'title' => '',
        'path' => '',
        'config' => array(
          'source' => 'user',
          'data provider' => 'FrxDrupal',
          'access callback' => 'user_access',
          'user callback' => 'forena_current_user_id'
        ),
      );
    }
    else {
      $repos = FrxReportGenerator::instance()->app->repositories();
      $r = $repos[$source];
      $load = FrxReportGenerator::instance()->repository($source);
      // Remove teh object from the data.
      unset($load['data']);
      $form_state['storage'] = array(
        'name' => $source,
        'title' => $r['title'],
        'path' => @$r['path'],
        'config' => $load,
      );

    }
  }
  $data = $form_state['storage'];
  $config = $data['config'];

  $locked = !($adding || (@$config['source'] == 'user'));
  $values = @$form_state['values'];

  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Machine readable name.  Used in referencing all data used by this source. must should not contain any special characters or spaces.'),
    '#disabled' => !$adding,
    '#default_value' => $data['name'],
    '#required' => $adding,
   );

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#required' => TRUE,
    '#description' => t('Human readable name that describes the data source.  This primarily occurs in error messages where the data source cannot be accessed.'),
    '#default_value' => $data['title'],
  );

  $form['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enabled'),
    '#description' => t('Disabling will cause all queries to return no data.'),
    '#default_value' => @$data['enabled']!==0,
  );

  $form['debug'] = array(
    '#type' => 'checkbox',
    '#title' => t('Debug'),
    '#description' => t('Write information to the screen and logs for each query executed.'),
    '#default_value' => @$config['debug'],
  );

  $form['path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path'),
    '#required' => !$locked,
    '#disabled' => $locked,
    '#description' => t('Directory containing data block files.'),
    '#default_value' => @$data['path'],
  );

  $user_options = array(
    '' => 'None',
    'forena_current_user_id' => 'UID',
    'forena_current_user_name' => 'User name',
  );

  $form['user_callback'] = array(
    '#type' => 'select',
    '#title' =>  'Current user',
    '#disabled' => $locked,
    '#description' => t('Can be refererenced as :current_user in each data block.'),
    '#options' => $user_options,
    '#default_value' => @$config['user callback'],
    '#disabled' => $locked,
  );


  // Access method list
  $access = array(
    'callback' => t('Use drupal permissions'),
    'block' => t('Match values provided by a data block. '),
  );

  $form['access_method'] = array(
    '#type' => 'select',
    '#options' => $access,
    '#disabled' => $locked,
    '#title' => t('Data security method'),
    '#default_value' => empty($config['access block']) ? 'callback' : 'block',
    '#description' => t('Specify how the ACCESS defined for a data block is to be interpreted. '),
    '#ahah' => array(
      'path' => 'admin/settings/forena/js/access_details',
      'event' => 'change',
      'method' => 'replace',
      'wrapper' => 'access-details',
    ),
  );

  $form['access_details'] = array(
    '#type' => 'fieldset',
    '#prefix' => '<div id="access-details">',
    '#suffix' => '</div>',
    '#title' => t('Details'),
  ) ;

  switch (!empty($values['access_method']) ? $values['access_method'] : $form['access_method']['#default_value']) {
    case 'block':
      $form['access_details']['access_block'] = array(
        '#type' => 'textfield',
        '#title' => 'Data block providing permissions list',
        '#disabled' => $locked,
 //       '#autocomplete_path' => 'forena/data_block/autocomplete',
        '#description' => t('The datablock to be used to interpret permissions.  This should return a single column of permissions based on the current user.   May be provided by another repository.'),
        '#default_value' => @$config['access block'],
      );
      break;
    default:
      $form['access_details']['access_callback'] = array(
        '#type' => 'item',
        '#title' => 'Access callback',
        '#disabled' => $locked,
        '#value' => @$config['access callback'],
      );
  }



  // Driver list
  $drivers = array(
    'FrxDrupal' => t('Drupal'),
    'FrxOracle' => t('Oracle Database'),
    'FrxPDO' => t('PDO other than Drupal '),
    'FrxPostgres' => t('Postgres Database'),
    'FrxMSSQL' => t('MSSQL Database'),
    'FrxFiles' => t('XML Files'),
  );

  $form['data_provider'] = array(
    '#type' => 'select',
    '#title' => t('Driver'),
    '#description' => t('Forena data connection type'),
    '#options' => $drivers,
    '#disabled' => $locked,
    '#default_value' => $config['data provider'],
    '#ahah' => array(
      'path' => 'admin/settings/forena/js/connection_info',
      'event' => 'change',
      'method' => 'replace',
      'wrapper' => 'conn-div',
    ),
  );

  $form['connection'] = array(
    '#type' => 'fieldset',
    '#tree' => TRUE,
    '#title' => 'Connection info',
   '#prefix' =>  '<div id="conn-div">',
   '#suffix' => '</div>',
  );
  $data_provider = (!empty($form_state['values']['data_provider']) ? $form_state['values']['data_provider'] : $config['data provider']);

  // Common controls used in mulitple providers.
  $uri = array(
    '#type' => 'textfield',
    '#title' => t('uri'),
    '#descripton' => t('Connection string: see appropriate php documentation for more details.'),
    '#default_value' => @$config['uri'],
    '#required' => !$locked,
    '#disabled' => $locked,
  );

  $user = array(
    '#type' => 'textfield',
    '#title' => t('User'),
    '#default_value' => @$config['user'],
  );
  $password= array(
    '#type' => 'password',
    '#title' => t('Password'),
    '#default_value' => @$config['password'],
   );

  switch ($data_provider) {
    case 'FrxDrupal':
      $form['connection']['uri'] = array(
        '#type' => 'item',
        '#title' => t('URI'),
        '#disabled' => $locked,
        '#value' => 'Determined by Drupal settings.php file',
      );
      break;
    case 'FrxMSSQL':
      $form['connection']['uri'] = $uri;
      $form['connection']['user'] = $user;
      $form['connection']['new_password'] = $password;
      $form['connection']['mssql_xml'] = array(
        '#type' => 'checkbox',
        '#disabled' => $locked,
        '#title' => t('Microsoft SQL native XML'),
        '#description' => t('Use for XML auto queries to generate XML.'),
        '#default_value' => $config['mssql_xml'],
      );
            break;
    case 'FrxOracle':
      $form['connection']['uri'] = $uri;
      $form['connection']['user'] = $user;
      $form['connection']['new_password'] = $password;
      $form['connection']['character_set'] = array(
        '#type' => 'textfield',
        '#title' => t('Character Set'),
        '#disabled' => $locked,
        '#description' => t('Leave blank for default character set'),
        '#default_value' => @$config['character_set'],

      );
      $form['connection']['oracle_xml'] = array(
        '#type' => 'checkbox',
        '#title' => t('Oracle native XML'),
        '#disabled' => $locked,
        '#description' => t('Use the function provided with Forena to generate XML.  Requires installing a function into the database'),
        '#default_value' => $config['oracle_xml'],
      );
      break;
    case 'FrxPDO':
      $form['connection']['uri'] = $uri;
       $form['connection']['user'] = $user;
       $form['connection']['new_password'] = $password;
       break;
    case 'FrxPostgres':
      $form['connection']['uri'] = $uri;
      $form['connection']['new_password'] = $password;
      $form['connection']['postgres_xml'] = array(
        '#type' => 'checkbox',
        '#title' => t('Postgres native XML'),
        '#disabled' => $locked,
        '#default_value' => @$config['postgres_xml'],
        '#description' => t('Use Postgres native XML support.  Requires Posgres 8.3 or better'),
      );

    default:
      $form['connection']['uri'] = $uri;
  }

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#submit' => array('forena_data_settings_save'),
  );


  $form['delete'] = array(
    "#type" => 'submit',
    '#value' => t('Delete'),
    '#submit' => array('forena_data_settings_delete'),
  );

  return $form;
}



/**
 * Submit handler to cause save events to happen.
 * Enter description here ...
 */
function forena_data_settings_save($form, &$form_state) {
  $values = $form_state['values'];
  foreach ($form['connection'] as $key => $element) {
  	if ($element['#disabled']) {
  		$values['connection'][$key] = $element['#default_value'];
  	}
  }
  foreach ($form as $key => $element) {
  	if ($element['#disabled']) {
  		$values[$key] = $element['#default_value'];
  	}
  }

  $name = $values['name'];
  $config = $form_state['storage']['config'];
  $config['path'] = @$values['path'];
  $config['data provider'] = $values['data_provider'];
  $config['user callback'] = $values['user_callback'];
  $config['debug'] = $values['debug'];
  if (@$values['connection']['new_password']) {
    $values['connection']['password'] = $values['connection']['new_password'];
  }
  if (isset($values['connection']['new_password'])) unset($values['connection']['new_password']);
  if (is_array(@$values['connection'])) $config = array_merge($config, @$values['connection']);

  if ($values['access_method']=='callback') {
    $config['access callback'] = empty($values['access_callback']) ? 'user_access' : $values['access_callback'];
    if (isset($config['access block'])) unset($config['access block']);
  }
  else {
    $config['access block'] = $values['access_block'];
  }
  $config_str = serialize($config);
  $result = db_query("SELECT * FROM {forena_repositories} WHERE repository = '%s'", $name);

  if ($result) drupal_set_message(t('Data connection settings saved'));
  if ($row = db_fetch_object($result)) {
    db_query("UPDATE {forena_repositories} SET title='%s', enabled=%d, config='%s'
      WHERE repository='%s'",
      $values['title'],
      $values['enabled'],
      $config_str,
      $name
     );

  }
  else {
    $form_state['#redirect'] = 'admin/forena/content/data';
    db_query("INSERT INTO {forena_repositories} (repository, title, enabled, config)
      VALUES ('%s', '%s', %d, '%s')",
        $name,
         $values['title'],
         $values['enabled'],
         $config_str
      );
  }
  unset($form_state['storage']);
  $form_state['redirect'] = array('admin/settings/forena/data');
}

/**
 * Delete the function
 * Enter description here ...
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_data_settings_delete($form, &$form_state) {
  if (!@$form_state['storage]']['locked']) {
    db_query("DELETE FROM {forena_repositories} WHERE repository='%s'",
    $form_state['values']['name']);
    $form_state['redirect'] = 'admin/settings/forena/data';
    unset($form_state['storage']);
  }

}
/**
 * Ajax callback for settings pages.
 * Enter description here ...
 * @param string $part matches the form path callback
 */
function forena_settings_js($part='') {
  $form = forena_ahah_helper_start();
	switch ($part) {
		case 'connection_info':
			$form =  $form['connection'];
			break;
		case 'access_details':
			$form = $form['access_details'];
		  break;
		default:
			$form = array();
	}
	forena_ahah_helper_end($form);
}

/**
 *
 * @param $form_state
 * @return a form to edit the fields of the report
 */
function forena_fields_form($form_state, $report_name) {
  $desc = forena_report_desc($report_name);
  if (!$desc['exists']) {
  	drupal_not_found();
  	exit;
  }
  $name = $desc['name'];
  $filename = $desc['filename'];
  @$format = $desc['format'];
  if ($name) {
    if (file_exists($filename)) {
      $r = forena_get_report_editor($name);
      drupal_set_title($r->title);
      $regexp = FRX_TOKEN_EXP;
      $match = array("{", "}");

      /*search the body, looking for {}*/
      $body = $r->simplexml->body->asXML();
      $head = $r->simplexml->head;

      $fields = array();
      preg_match_all($regexp, $body, $out);
      foreach ($out as $key => $value) {
        foreach ($value as $el) {
          $clean_element = str_replace($match, "", $el);
          $fields[$clean_element] = $clean_element;
        }
      }

      $form = array();

      $form['report_name'] = array(
        '#type' => 'value',
        '#value' => $name,
      );

      $form['fields'] = array(
        '#tree' => TRUE,
      );
      /*Now check the fields in the body against the xml*/
      $i=0;
      foreach ($fields as $field) {
        $i++;
        $field_ids[$i] = $field;
        $form['fields'][$i] = array(
          '#tree' => TRUE,
          '#type' => 'fieldset',
          '#title' => $field,
          '#collapsible' => TRUE,
          '#collapsed' => TRUE,
        );

        $path = 'frx:fields/frx:field[@id="' . $field . '"]';
        $node =  $head->xpath($path);
        $attr= array();
        $default = '';
        if ($node) {
          $attr = $node[0];
          $default = (string) $node[0];
        }

        $form['fields'][$i]['id'] = array(
          '#type' => 'value',
          '#value' => $field,
        );

        $form['fields'][$i]['format'] = array(
          '#type' => 'textfield',
          '#title' => t('format'),
          '#default_value' => @$attr['format'],
          '#size' => 30,
          '#autocomplete_path' => 'forena/fields/format/autocomplete',
          '#description' => t('Format a date and time field by entering the name of a supported format function. Enter a "*" to see all available formats.')
        );

        $form['fields'][$i]['format-string'] = array(
          '#type' => 'textfield',
          '#title' => t('format-string'),
          '#default_value' => @$attr['format-string'],
          '#size' => 30,
          '#description' => t('The display type of your format.')
        );

        $form['fields'][$i]['link'] = array(
          '#type' => 'textfield',
          '#title' => t('link'),
          '#default_value' => @$attr['link'],
          '#size' => 100,
          '#maxlength' => 256,
          '#description' => t('Create a link that incorporates this field, e.g "profile/{field_name}" will create a link to this field_name\'s profile. *Note the field must be wrapped in {}.')
        );

        $form['fields'][$i]['target'] = array(
          '#type' => 'textfield',
          '#title' => t('target'),
          '#default_value' => @$attr['target'],
          '#size' => 30,
          '#description' => t('Link target eg. _BLANK')
        );

        $form['fields'][$i]['default'] = array(
          '#type' => 'textfield',
          '#title' => t('default value'),
          '#default_value' => $default,
          '#size' => 30,
          '#description' => t('The value to be displayed in the report in the place of the field.')
        );
      }
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


function forena_fields_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $name = $values['report_name'];

  $r = forena_get_report_editor($name);
  /*now build the fields*/
  $fields = $values['fields'];
  $r->setFields($fields);

  if (forena_save_report($name, $r->asXML(), TRUE) == 1) {
    drupal_set_message(t('Your report, %s has been saved.', array('%s' => $name)));
  }
  else {
    drupal_set_message(t('There was an error saving your report, %s to the database.', array('%s' => $name)));
  }
}

function forena_add_data_block_form( &$form_state, $report_name) {
  // If no step is set then we're visiting this form for the first time.
  if (!isset($form_state['storage']['step'])) {
    $desc = $form_state['storage']['desc'] = forena_report_desc($report_name);
    $filename = $form_state['storage']['filename'] = $desc['filename'];
    $format = isset($desc['format']) ? $desc['format'] : '';
    $r = forena_get_report_editor($desc['name']);
    $form_state['storage']['title'] = $r->title;
    if(!@$desc['exists']) {
      drupal_not_found();
      return;
    }
    $step = $form_state['storage']['step'] = 'select_block';
  }
  else {
    $desc = $form_state['storage']['desc'];
    $step = $form_state['storage']['step'];
  }

  // General report data variables;
  drupal_set_title($form_state['storage']['title']);
  $form = array();
  $template_array = FrxReportGenerator::instance()->supported_templates();
  @$default_template = (isset($form_state['values']['templates'])) ? $form_state['values']['templates'] : $form_state['storage']['template'];
  $template = ($default_template) ? $default_template : 'FrxTable';
  $template_obj = FrxReportGenerator::instance()->get_templates($template);
  $params = @$form_state['storage']['parameters'];
  $param_values = @$form_state['storage']['parms'];
  $form = array();
  $form['report_name'] = array(
      '#type' => 'value',
      '#value' => $desc['name'],
  );

  $form['return'] = array(
    '#type' => 'value',
    '#value' => $desc['link'] . '/edit/data',
  );

  if ($step == 'select_block') {
    $form['data_block'] = array(
      '#type' => 'textfield',
      '#title' => t('Data Block'),
      '#default_value' => @$form_state['storage']['data_block'],
      '#autocomplete_path' => 'forena/data_block/autocomplete',
      '#description' => t('Enter a data block, a preview will appear below'),
    );

    $form['parameters'] = array(
      '#type' => 'textfield',
      '#title' => t('Parameter data'),
      '#description' => t('Enter parameters to be used for the block as they would appear on a url'),
      '#default_value' => @$form_state['storage']['parm_values'],
    );

    $form['where_clause'] = array(
      '#type' => 'textfield',
      '#title' => t('Where Clause'),
      '#default_value' => @$form_state['storage']['where_clause'],
      '#description' => t('Enter a where clause to filter your report.'),
    );
    //If there are parameters in the data block allow the user to input data
    //Check if there is already a parameter of the same name in the existing report. If there is, use the report value.
    if ($params) {
      $rpt_params = array();
      //returns a 2 dimensional array of report parameters from the report
      $param_values = @$form_state['values']['parameters'];

      $form['parms'] = array(
        '#tree' => TRUE,
        '#title' => 'Parameters',
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        '#collapsed' => (@$form_state['storage']['xml']) ? TRUE : FALSE,
      );

      foreach ($params as $param) {
        $form['parms'][$param] = array(
          '#type' => 'textfield',
          '#title' => t($param),
          '#default_value' => (isset($param_values[$param])) ? $param_values[$param] : '',
        );
      }
    }
    $form['preview'] = array(
      '#type' => 'submit',
      '#value' => 'Preview',
      '#validate' => array('forena_data_select_block_validate'),
      '#submit' => array('forena_data_select_block_submit'),
    );
  }

  //Template selection
  if ($step == 'select_template') {
    $form['templates'] = array(
      '#type' => 'select',
      '#title' => t('Templates'),
      '#default_value' => $template,
      '#options' => $template_array,
      '#required' => TRUE,
      '#description' => t('Select a template to preview the data block in.'),
    );

    forena_template_ajax($form['templates']);

    $form['template'] = array(
     '#prefix' => '<div id="template-wrapper">',
     '#suffix' => '</div>',
    );

   $config = isset($form_state['values']['template_data']) ? @$form_state['values']['template_data']['config'] : @$form_state['storage']['config'];
   $config['block'] = $form_state['storage']['data_block'];
   $config['clause'] = $form_state['storage']['where_clause'];



    $xml = $form_state['storage']['xml'];
    if ($xml) {

        $xml = new SimpleXMLElement($xml);
        //create xml from template object
        $template_obj = FrxReportGenerator::instance()->get_templates($template);
        if ($template_obj) {

          if ($template_obj) {
            $form['template']['template_data'] = array(
              '#type' => 'fieldset',
              '#title' => t('Template Settings'),
              '#tree' => TRUE,
            );
            $form['template']['template_data']['config'] = $template_obj->config_form($config, $xml);
          };

          $template_obj->generate($xml, $config);
          $body = $template_obj->template();
          $rpt_xml = $template_obj->asXML();
          $parms = $form_state['storage']['parms'];
          $output = forena_render_report($rpt_xml, NULL, $parms);
          //render the xml
          $form_state['storage']['output'] = $output;
          $form_state['storage']['template_body'] = $body;
        }
      }
    if (@$form_state['storage']['output'] != '') {
      $form['template']['add'] = array(
        '#type' => 'submit',
        '#value' => 'Add',
        '#submit' => array('forena_data_block_add'),
      );
    }

    $form['template']['preview'] = array(
      '#type' => 'submit',
      '#value' => 'Preview',
      '#submit' => array('forena_data_select_template_submit'),
    );



    if (isset($form_state['storage']['output'])) {
      $form['template']['output_text'] = array(
        '#type' => 'markup',
        '#value' => '</p>' . $form_state['storage']['output'] . '</p>',
      );
    }

    $form['cancel'] = array(
      '#type' => 'submit',
      '#value' => 'Cancel',
      '#submit' => array('forena_add_cancel'),
    );
  }
  return $form;

}

function forena_data_select_template_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['template'] = $form_state['values']['templates'];
}

/**
 * Ajax callback for settings pages.
 * Enter description here ...
 * @param string $part matches the form path callback
 */
function forena_template_info_js($part='') {
  $form = forena_ahah_helper_start();
  $form = $form['template'];
	forena_ahah_helper_end($form);
}

function forena_template_info_callback($form, &$form_state) {
  return $form['template'];
}

function forena_add_cancel($form, &$form_state) {
	unset($form_state['storage']);
  $form_state['redirect'] = $form_state['values']['return'];
}

/**
 * submit handler for forena_data_block_form.
 * This adds the datablock to an existing report.
 *
 */
function forena_data_block_add($form, &$form_state) {
  //$form_state['rebuild'] = TRUE;
  $values = $form_state['values'];
  $report_name = $values['report_name'];
  $data_block = $form_state['storage']['data_block'];
  $where_clause = $form_state['storage']['where_clause'];
  $r = forena_get_report_editor($report_name);
  $head = $r->simplexml->head;

  //Get the xml body of the added data block
  $added_report = $form_state['storage']['template_body'];

  //Get the data block parameters
  $block_info = FrxReportGenerator::instance()->load_block($data_block, $where_clause);
  $data_block_parms = $block_info['tokens'];

  //Build the parameters
  $xml = "";
  $path_parent = '//frx:parameters';
  $parent_node = $head->xpath($path_parent);

  $path_child = '//frx:parm';
  $child_nodes = $head->xpath($path_child);
  $found_rpt_param = FALSE;

  $parm_name = 'parm';
  //loop through the data block parameters
  foreach ($data_block_parms as $index => $id) {
    //check if rpt_params exist
    if ($child_nodes) {
      $path_id = '//frx:parm[@id="' . $id . '"]';
      $found_rpt_param = $child_nodes[0]->xpath($path_id);
    }
    //add block params if none are found in report
    if (!$found_rpt_param) {
      if ($parent_node) {
        $parm = $parent_node[0]->addChild($parm_name);
        $parm->addAttribute('id', $id);
      }
    }
  }

  $main_report = forena_inner_xml($r->simplexml, 'body');
  $added_report = forena_clean_xhtml($added_report);

  $r->setBody($main_report . "\n" . $added_report);

  if (forena_save_report($report_name, $r->asXML(), TRUE) == 1) {
    drupal_set_message(t('Your report has been saved'));
    unset($form_state['storage']);
    $form_state['redirect'] = $values['return'];
  }
}

/**
 * Validates the forena_data_block_form's data_block field.
 * @param $form
 * @param $form_state
 * @return unknown_type
 */
function forena_data_select_block_validate($form, &$form_state) {
  $values = $form_state['values'];
  $block_info = array();
  $data_block = $values['data_block'];
  //validate the data block only if delete block was not chosen
  if ($data_block) $block_info = FrxReportGenerator::instance()->load_block($data_block);
  if (!$block_info) {
    form_set_error('data_block', t('Invalid data block.'));
  }
}

function forena_data_select_block_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $values = $form_state['values'];
  $name = $values['report_name'];
  $data_block = $form_state['storage']['data_block'] = $values['data_block'];
  $params = $form_state['storage']['params'] = isset($values['params']) ? $values['params'] : array();
  $where_clause = $form_state['storage']['where_clause'] = $values['where_clause'];
  $parameters = $form_state['storage']['parm_data'] = $values['parameters'];
  $parms = isset($values['parms']) ? $values['parms']: array();
  $xml='';
  $block_info='';

  if ($data_block)  {
    //get xml from data block
    //check for and store parameters
    $block_info = FrxReportGenerator::instance()->load_block($data_block, $where_clause);
    if ($block_info) {
      //if there were not any parameters passed. Use the report parameters
      if (!$parms) {
        $r = FrxReportGenerator::instance()->get_report($values['report_name']);
        $rpt_params = $r->parameters;
        if ($rpt_params) foreach ($block_info['tokens'] as $index => $id) {
          $parms[$id] = (string)@$rpt_params[$id];
        }
      }

			drupal_alter('forena_parameters', $name, $parms );


      //now invoke the data provider with the correct params
      FrxData::instance()->push($parms, 'parm');
      $form_state['storage']['parms'] = $params;
      $xml = FrxReportGenerator::instance()->invoke_data_provider($data_block, $parms, $where_clause, $parameters);
      if ($xml) {
        $form_state['storage']['step'] = 'select_template';
        $form_state['storage']['xml'] = $xml->asXML();
        $form_state['storage']['parms'] = $parms;
      }
      else $form_state['storage']['parameters'] = @$block_info['tokens'];
      //create xml from template object
    }
  }
}

function forena_add_data_block_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $template = $form_state['storage']['template'] = @$values['template'];
  $name = $values['report_name'];
  if ($values['template_data']) $form_state['storage']['template_config'] = $values['template_data']['config'];
  $form_state['rebuild'] = TRUE;

}

/**
 * A form to preview and add data blocks to an existing report
 * @param unknown_type $form_state
 * @return unknown_type
 */
function forena_data_block_form(&$form_state, $report_name) {
  $desc = forena_report_desc($report_name);
  if (!$desc['exists']) {
  	drupal_not_found();
  	exit;
  }
  $name = $desc['name'];
  $filename = $desc['filename'];
  $format = isset($desc['format']) ? $desc['format'] : '';
  $r = forena_get_report_editor($name);
    drupal_set_title($r->title);
    $form = array();
    $title = (string)$r->title;

    $template_array = FrxReportGenerator::instance()->supported_templates();
    @$default_template = $form_state['storage']['template'];
    @$params = $form_state['storage']['parameters'];
    @$param_values = $form_state['storage']['param_values'];

    $form = array();

    $form['report_name'] = array(
        '#type' => 'value',
        '#value' => $name,
     );
    $links = array();
    $links[] = array('href' => $desc['link']. '/edit/data/add', 'title' => 'Add Data' );
    $form['add_link'] = array(
      '#type' => 'markup',
      '#value' => theme_links( $links,array('class' => 'action-links'))
    );


    //The default submit handler
    //If someone presses enter, this handler will execute
    $form['default_submit'] = array(
      '#type' => 'submit',
      '#submit' => array('forena_data_block_form_submit'),
      '#prefix' => '<div style="visibility:hidden;float:right">',
      '#suffix' => '</div>',
    );

    //find the datablocks in the existing report
    $data_block_array = array();
    if ($r) $r->get_attributes_by_id();
    $body = $r->simplexml->body;
    $path = '//*[@frx:block]';

    $form['blocks'] = array(
      '#tree' => TRUE,
    );

    if ($body) foreach ($body->xpath($path) as $node) {
      $attrs = $node->attributes(FRX_NS);
      $id = (string)$node['id'];
      $block = (string)$attrs['block'];
      $parameters = (string)$attrs['parameters'];
      $clause = ($attrs['clause']) ?  "(" . (string)$attrs['clause'] . ")" : '';
      $block_info = FrxReportGenerator::instance()->load_block($block, $clause);
      $access = $block_info['access'];
      $data_block_array[$id] = $block . ": <i>" . $clause . ' security "' . $access . '"</i>';

      $ctl = array(
        '#type' => 'fieldset',
        '#title' => $id,
        '#collapsible' => true,
      );

      $ctl['id'] = array(
        '#type' => 'value',
        '#value' => $id,
      );

      $ctl['block'] = array(
        '#type' => 'textfield',
        '#autocomplete_path' => 'forena/data_block/autocomplete',
        '#title' => 'Data Block',
        '#default_value' => $block,
        '#description' => t('The data block to be used as the source for the data in the report.'),
      );

      $ctl['access'] = array(
        '#type' => 'item',
        '#value' => $access,
        '#title' => t('Security'),
      );

      $ctl['parameters'] = array(
        '#type' => 'textfield',
        '#title' => t('Parameters'),
        '#default_value' => $parameters,
        '#description' => t('Leave blank to use the current data context as parameters.  In a simple report this will be the parameters of the report.
          In master detail reports, this will be the report of the containing data block. Parameters should be specified in the form of a url string.')
      );

      $ctl['delete'] = array(
        '#type' => 'checkbox',
        '#title' => 'Confirm delete',
        '#description' => t('Check this box to remove this data from the report.'),
      );

      $ctl['delete_submit'] = array(
        '#type' => 'submit',
        '#value' => 'Delete',
        '#submit' => array('forena_data_block_delete'),
      );

      $form['blocks'][] = $ctl;
    }

    $form['save'] = array(
      '#type' => 'submit',
      '#value' => 'Save',
    );
    return $form;
}


/**
 * The Preview submit handler for forena_add_block_form
 * Renders datablock into a report
 * @param $form
 * @param $form_state
 * @return unknown_type
 */
function forena_data_block_form_submit($form, &$form_state) {
  $xml='';
  $block_info='';
  $values = $form_state['values'];
  $name = $values['report_name'];
  $r = forena_get_report_editor($name);

  if ($values['blocks']) {
    $r->get_attributes_by_id();
    $data = array();
    foreach ($values['blocks'] as $block) {
      if (!$block['delete']) {

        $attrs = $block;
        unset($attrs['delete']);
        unset($attrs['delete_submit']);
        unset($attrs['id']);
        $data[$block['id']] = $attrs;
      }
    }
    if ($r) $r->save_attributes_by_id($data);
  }
  if (forena_save_report($name, $r->asXML(), TRUE) == 1) {
    drupal_set_message(t('Your report %s been saved', array('%s' => $name)));
  }
}


/**
 * Delete submit handler to delete report blocks
 * @param $form
 * @param $form_state
 * @return unknown_type
 */
function forena_data_block_delete($form, &$form_state) {
  $values = $form_state['values'];

  $report_name = $values['report_name'];
  $r = FrxReportGenerator::instance()->get_report($report_name);

  //find the datablocks in the existing report
  if ($r) $r->get_attributes_by_id();
  $body = $r->body;

  foreach ($values['blocks'] as $block) {
    if ($block['delete']) {
      if ($r) $r->deleteNode($block['id']);
    }
  }

 if (forena_save_report($report_name, $r->rpt_xml, TRUE) == 1) {
    drupal_set_message(t('Your report has been saved.'));
  }
}
/**
 * Translations Dispaly.
 * @param unknown_type $name_in
 */
function forena_report_translations($name_in='') {
  $desc = forena_report_desc($name_in);
  global $language;
  $cur_lang = $language->language=='en' ? '' : $language->language;
  $name = $desc['name'];
  $filename = $desc['filename'];
  $languages = language_list();

  @list($lang, $base_name) = explode('/', $name, 2);
  if (array_key_exists($lang, $languages)) {
    $name = $base_name;
  }
  elseif ($cur_lang &&  file_exists(FrxReportGenerator::instance()->report_path() . '/' . $cur_lang . '/' . str_replace('.', '/', $name_in) . '.frx')) {
    $name_in = $cur_lang . '.' . $name_in;
  }
  $data = array();
  $headers = array(t('Language'), t('Title'), t('Last Modified'), t('Operation'));
  // Generate list of all existing tranlsations and their status.
  $report_path = FrxReportGenerator::instance()->report_path();
  foreach ($languages as $key=>$lang) {
    $i_name = ($key=='en') ? $name : $key . '/' . $name;
    $i_link = $key . '.' . $name;
    if (file_exists($report_path . '/' . $i_name . '.frx')) {
      $modified = format_date(filemtime($report_path . '/' . $i_name . '.frx'));
      $r = forena_get_report_editor($i_name);
      $title = $r->title;
      $edit = l(t('Edit'), 'reports/' . str_replace('/', '.', $i_link) . '/edit',array('query' => array('language' => $key)));
    }
    else {
      $modified = '';
      $title ='';
      //$language = $cur_lang;
      $edit = l(t('Add'), 'reports/' . $name_in . '/translate', array('query' => array('new_language' => $key)));
    }
    $data[] = array($lang->native, $title, $modified, $edit);

  }

  return theme_table( $headers, $data);
}

/**
 * Form to confirm the delete of a form.
 * @param $form_state
 * @return unknown_type
 */
function forena_delete_form($form_state, $name_in) {
  // Parse names from url
  $report_name = $name_in;
  $desc = forena_report_desc($report_name);
  $report_name = $desc['name'];
  global $language;

  $link = $desc['link'];
  $form['report_name'] = array(
    '#type' => 'value',
    '#value' => $report_name
  );
  if (module_exists('locale')) {

  	$options = array('all' => t('All Languages'),
  	  'single' => t('Only %lang', array('%lang' => $language->native)),
  	);
  	$form['delete_all_languages'] = array(
  	  '#type' => 'radios',
  	  '#title' => t('Delete report in all languages?'),
  	  '#options' => $options,
  	  '#default_value' => 'single',
   	);
   	$form['base_name'] = array(
   	  '#type' => 'value',
   	  '#value' => $desc['base_name'],
   	);
  }
  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $report_name)),
    isset($_GET['destination']) ? $_GET['destination'] : $link,
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
  return $form;
}

function forena_delete_form_submit($form, &$form_state) {
	$values = $form_state['values'];
  $report_name = $values['report_name'];
  if (isset($values['delete_all_languages']) && $values['delete_all_languages']=='all') {
  	foreach(language_list() as $language) {
  		$report_name = $language->language . '.' . $values['base_name'];
  		$desc = forena_report_desc($report_name);
  		forena_delete_report($desc['name']);
  	}
  	drupal_set_message(t('Deleted %s', array('%s' => $values['base_name'])));
  }
  else {
    drupal_set_message(t('Deleted %s', array('%s' => $report_name)));
    forena_delete_report($report_name);
  }
  unset($form_state['storage']);
  $form_state['redirect'] = 'forena';
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
      'indent-spaces'             => 2,
      'indent' => 'auto',
      'input-xml'          => TRUE,
      'output-xml'         => TRUE,
      'indent-attributes'  => FALSE,
      'indent-spaces'      => 2,
      'add-xml-space'      => TRUE,
      'wrap'               => 135 );
    $ret = tidy_repair_string($xhtml, $config, 'utf8');
  }
  else {
    $ret = str_replace('&nbsp;', '&#160;', $ret);
  }
  return $ret;
}

/**
 * Provides list of blocks that a user has access to that are in any repository matching
 * a specified search string
 *
 * @param string $search block to search for.
 * @return array list of blocks the user has access to.
 */
function forena_user_data_blocks($search) {
  $repos = FrxReportGenerator::instance()->app->repositories();
  foreach ($repos as $name => $r) {

    // Make really sure the data provider objects have been instantiated
    $provider = $r;
    if (!@$provider['data']) {
      $provider = FrxReportGenerator::instance()->repository($name);
    }
    $repos[$name] = $provider;
    // Invoke the list block function to find out all of the block names
    $o = $provider['data'];
    if (method_exists($o, 'list_blocks')) {
      $blocks = $o->list_blocks($search);
      foreach ($blocks as $block) {
        $block_info = $o->load_block($block);
        if (method_exists($o, 'access')) {
          $allow = $o->access($block_info['access']);
          if ($allow) $user_blocks[] = $name . '/' . $block;
        }
      }
    }
  }
  return $user_blocks;
}

/**
 *  Prettifies an XML string into a human-readable and indented work of art
 *  @param string $xml The XML as a string
 *  @param boolean $html_output True if the output should be escaped (for use in HTML)
 */
function forena_xmlpp($xml, $html_output=FALSE) {
    $xml_obj = new SimpleXMLElement($xml);
    $level = 4;
    $indent = 0; // current indentation level
    $pretty = array();

    // get an array containing each XML element
    $xml = explode("\n", preg_replace('/>\s*</', ">\n<", $xml_obj->asXML()));

    // shift off opening XML tag if present
    if (count($xml) && preg_match('/^<\?\s*xml/', $xml[0])) {
      $pretty[] = array_shift($xml);
    }

    foreach ($xml as $el) {
      if (preg_match('/^<([\w])+[^>\/]*>$/U', $el)) {
          // opening tag, increase indent
          $pretty[] = str_repeat(' ', $indent) . $el;
          $indent += $level;
      }
      else {
        if (preg_match('/^<\/.+>$/', $el)) {
          $indent -= $level;  // closing tag, decrease indent
        }
        if ($indent < 0) {
          $indent += $level;
        }
        $pretty[] = str_repeat(' ', $indent) . $el;
      }
    }
    $xml = implode("\n", $pretty);
    return ($html_output) ? htmlentities($xml) : $xml;
}

function forena_admin_params_form($form_state, $report_name) {

  $desc = forena_report_desc($report_name);
  if (!$desc['exists']) {
  	drupal_not_found();
  	exit;
  }
  $name = $desc['name'];
  $filename = $desc['filename'];
  $format = @$desc['format'];
  if ($desc['exists'] && $name) {
    $r = forena_get_report_editor($name);
    drupal_set_title($r->title);
    $form = array();

    $form['rpt_name'] = array(
      '#type' => 'value',
      '#value' => $name,
    );

    $links = array();
    $links[] = array('href' => $desc['link']. '/edit/params/add', 'title' => 'Add Parameter' );
    $form['add_link'] = array(
      '#type' => 'markup',
      '#value' => theme_links($links, array('class' => 'action-links'))
    );

    if ($r) $nodes = $r->simplexml->xpath('head/frx:parameters/frx:parm');
    if ($nodes) {
      $form['params'] = array(
        '#tree' => TRUE,
      );

      foreach ($nodes as $node) {
        $attrs = $node->attributes();
        $id = (string)$attrs['id'];
        $label = (string)$attrs['label'];
        $value = (string)$attrs['value'] ? (string)$attrs['value'] : (string)$node;
        $require = (string)$attrs['require'];
        $data_source = (string)$attrs['data_source'];
        $data_field = (string)$attrs['data_field'];
        $desc = (string)$attrs['desc'];
        $type = (string)$attrs['type'];

        //make a subtree for each param
        $form['params'][$id] = array(
          '#tree' => TRUE,
          '#type' => 'fieldset',
          '#title' => ($label) ? $label : $id,
          '#collapsible' => TRUE,
          '#collapsed' => TRUE,
        );

        $form['params'][$id]['id'] = array(
          '#type' => 'value',
          '#value' => $id,
        );

        $form['params'][$id]['label'] = array(
          '#type' => 'textfield',
          '#title' => t('label'),
          '#default_value' => $label,
          '#required' => FALSE,
          '#description' => t('A descriptive name of the parameter to be displayed on a form.')
        );

        $form['params'][$id]['require'] = array(
          '#type' => 'radios',
          '#title' => t('require'),
          '#default_value' => ($require) ? $require : "0",
          '#options' => array("1" => t('Yes'), "0" => t('No')),
          '#description' => t('Requires a value for this parameter to display the report. If there is not a default value, the user will be prompted to enter a value.')
        );

        $form['params'][$id]['value'] = array(
          '#type' => 'textfield',
          '#title' => 'default value',
          '#default_value' => $value,
          '#description' => t('The value of the parameter for the initial view of the report.')
        );

        $form['params'][$id]['desc'] = array(
          '#type' => 'textfield',
          '#title' => t('description'),
          '#default_value' => $desc,
          '#description' => t('Enter a helpful description about this parameter. This will display on the form when the user is prompted to enter a parameter.'),
        );

        $form['params'][$id]['data_source'] = array(
          '#type' => 'textfield',
          '#title' => t('Data Source'),
          '#default_value' => $data_source,
          '#autocomplete_path' => 'forena/data_block/autocomplete',
          '#description' => t('Data block to be used for list of values'),
        );

        $form['params'][$id]['data_field'] = array(
          '#type' => 'textfield',
          '#title' => t('Data Field'),
          '#default_value' => $data_field,
          '#description' => t('Column in data block to be used for ID'),
        );

         $form['params'][$id]['type'] = array(
          '#type' => 'select',
          '#title' => t('Input Control Type'),
          '#default_value' => $type,
          '#options' => array('textfield' => 'Text', 'radios' => 'Radios', 'select' => 'Select','multiselect' => 'Multi-Select', 'checkboxes' => 'Checkboxes'),
          '#description' => t('Enter a helpful description about this parameter. This will display on the form when the user is prompted to enter a parameter.'),
        );

        $form['params'][$id]['delete_ind'] = array(
          '#type' => 'checkbox',
          '#title' => 'Remove this parameter',
          '#description' => t('Check to remove this parameter, then save'),
        );
      }

      $form['submit'] = array(
       '#type' => 'submit',
       '#value' => 'Save',
      );
    }
    return $form;
  }
  else {
    drupal_not_found();
  }
}

function forena_add_param_form(&$form_state, $report_name) {
  $desc = forena_report_desc($report_name);
  $name = $desc['name'];
  $filename = $desc['filename'];
  $format = @$desc['format'];
  if ($desc['exists'] && $name) {
    $r = forena_get_report_editor($name);
    drupal_set_title($r->title);
    $form = array();

    $form['rpt_name'] = array(
      '#type' => 'value',
      '#value' => $name,
    );

    $form['return'] = array(
      '#type' => 'value',
      '#value' => $desc['link'] . '/edit/params',
    );

    $form['params'] = array('#tree' => TRUE);

    $form['params']['id'] = array(
      '#type' => 'textfield',
      '#title' => t('id'),
      '#required' => TRUE,
      '#description' => t('The name of the parameter to be filtered against.')
    );

    $form['params']['label'] = array(
      '#type' => 'textfield',
      '#title' => t('label'),
      '#required' => FALSE,
      '#description' => t('A descriptive name of the parameter to be displayed on a form.')
    );

    $form['params']['require'] = array(
      '#type' => 'radios',
      '#title' => t('require'),
      '#options' => array("1" => t('Yes'), "0" => t('No')),
      '#description' => t('Requires a value for this parameter to display the report. If there is not a default value, the user will be prompted to enter a value.')
    );

    $form['params']['value'] = array(
      '#type' => 'textfield',
      '#title' => 'default value',
      '#description' => t('The value of the parameter for the initial view of the report.')
    );

    $form['params']['desc'] = array(
      '#type' => 'textfield',
      '#title' => t('description'),
      '#description' => t('Enter a helpful description about this parameter. This will display on the form when the user is prompted to enter a parameter.'),
    );

    $form['save'] = array(
      '#type' => 'submit',
      '#value' => 'Add',
      '#submit' => array('forena_add_param_form_submit'),
    );

    $form['cancel'] = array(
      '#type' => 'submit',
      '#value' => 'Cancel',
      '#submit' => array('forena_add_cancel'),
    );
  }
  return $form;
}

function forena_add_param_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $name = $values['rpt_name'];
  $param = $values['params'];

  $r = forena_get_report_editor($name);
  //add the new parms
  $r->addParameter($param);

  $new_report = $r->asXML();

  if (forena_save_report($name, $new_report, TRUE) == 1) {
    drupal_set_message(t('Parameter added'));
    $form_state['redirect'] = $values['return'];
    unset($form_state['storage']);
  }
}

function forena_admin_params_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $name = $values['rpt_name'];
  $params = $values['params'];

  $r = forena_get_report_editor($name);
  //add the new parms
  $r->setParameters($params);
  //Remove paramters to delete
  foreach ($params as $parm) {
    if ($parm['delete_ind']) {
      $r->removeParm($parm['id']);
    }
  }

  $new_report = $r->asXML();

  if (forena_save_report($name, $new_report, TRUE) == 1) {
    drupal_set_message(t('Your report has been saved'));
  }
}

/**
 * Auto complete function for categories
 * Checks access for security as well.
 *
 * @param $string = string to be matched against categories
 * @return An array containing all matching categories
 */
function forena_get_categories($string='') {
  $result = db_query("SELECT * FROM {forena_reports} where hidden=0 AND category LIKE '%%%s%%' ORDER BY category, title asc", $string);
  $categories = array();
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
      if ($access && !$categories[$row->category]) {
        $categories[$row->category] = $row->category;
      }
    }
    return $categories;
}

function forena_template_ajax(&$form_element) {
	$form_element['#ahah'] =  array(
        'path' => 'forena/js/template',
        'callback' => 'forena_template_info_callback',
        'wrapper' => 'template-wrapper',
        'method' => 'replace',
        'event' => 'change',
      );
}

function forena_query_string($var) {
   return drupal_query_string_encode($var);
}
