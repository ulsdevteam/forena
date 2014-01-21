<?php
// $Id$
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
  $data = array();
  $links[] = array('href' => 'reports/add', 'title' => 'Create New Report' );
  $content = drupal_get_form('forena_sync_form');
  $output = drupal_render($content);
  $output .= theme('links',
    array(
      'links' => $links,
      'attributes' => array('class' => 'action-links'),
    )
  );
  // Add Data tables if it exists.

  drupal_add_css(drupal_get_path('module', 'forena') . '/forena.css');
  if (file_exists('sites/all/libraries/dataTables/media/js/jquery.dataTables.min.js')) {
    drupal_add_js(drupal_get_path('module', 'forena') . '/forena.admin.js');
    drupal_add_js('sites/all/libraries/dataTables/media/js/jquery.dataTables.min.js');
  }
  $headers = array(t('title'), t('name'), t('category'), t('operation'));
  $result = db_query('SELECT * FROM {forena_reports} where language=:language ORDER BY category,title', array(':language' => $language->language));
  foreach ($result as $row) {
    $rpt = str_replace('/', '.', $row->report_name);
    $edit = l(t('Edit'), 'reports/' . $rpt . '/edit');
    $clone = l(t('Clone'), 'reports/add/' . $rpt);
    $delete = l(t('Delete'), 'reports/' . $rpt . '/delete', array('query' => array('destination' => 'admin/structure/reports')));
    $title = l(t($row->title), 'reports/' . $rpt);
    $data[] = array($title, $row->report_name, $row->category, $edit . ' ' . $clone . ' ' . $delete);
  }
  $output .= '<div id="forena-reports-list">';
  $output .= theme_table(array('header' => $headers, 'rows' => $data, 'attributes' => array('class' => array( 'dataTable-paged')), 'caption' => '', 'sticky' => TRUE, 'colgroups' => array(), 'empty' => ''));
  $output .= '</div>';
  return $output;
}

/**
 * Remove the report from the database and file system.
 * @param string $report_name
 */
function forena_delete_report($report_name, $delete_file = TRUE) {
  $report_path = forena_report_path();
  $language = 'en';


  $filepath = $report_path . '/' . $report_name . '.frx';
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
    db_delete('forena_reports')
    ->condition('report_name', $report_name)
    ->condition('language', $language)
    ->execute();
  }
  else {
    drupal_set_message(t('Unable to delete file %s', array('%s' => $info['basename'])), 'error');
  }
}




function forena_filter_element($fmt, $name) {
  global $user;
  $element['format'] =  array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => $fmt!='',
    '#title' => t('Input formats'),
  );
  if (!$fmt) $fmt='full_html';
  // Get a list of formats that the current user has access to.
  $formats = filter_formats($user);
  foreach ($formats as $format) {
    $options[$format->format] = $format->name;
    $element['format']['guidelines'][$format->format] = array(
      '#theme' => 'filter_guidelines',
      '#required' => TRUE,
      '#format' => $format,
    );
  }
  $element['format']['guidelines']['#weight']=12;

  $element['format'][$name] = array(
    '#type' => 'select',
    '#title' => t('Text format'),
    '#options' => $options,
    '#default_value' => $fmt,
    '#access' => count($formats) > 1,
    '#weight' => 10,
    '#attributes' => array('class' => array('filter-list')),
  );

  $element['format']['help'] = array(
    '#type' => 'container',
    '#theme' => 'filter_tips_more_info',
    '#attributes' => array('class' => array('filter-help')),
    '#weight' => 11,
  );
  return $element['format'];
}


/**
 * Accepts the name of a file
 *
 * Returns an editor object of the file.
 *
 */
function forena_get_report_editor($report_name) {
  require_once('FrxEditor.inc');
  if ($report_name) {
    $r_text='';
    $report_path = forena_report_path();
    $filename = $report_path . '/' . $report_name . '.frx';
    if (file_exists($filename)) {
      $r_text = file_get_contents($filename);
    }
    $r = new FrxEditor($r_text);
    return $r;
  }
  else {
    return new FrxEditor();
  }
}

/**
 * Report syncronization form
 * @param $formid
 * @param $form_state
 */
function forena_sync_form($formid, &$form_state) {

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

  return $form;

}

/**
 * Forena admin settings form
 *
 */
function forena_settings() {
  $skins = variable_get('forena_skins', array());

  $report_path = forena_report_path();


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


  $form['forena_input_format'] = forena_filter_element(variable_get('forena_input_format', filter_default_format()), 'forena_input_format');
  $form['forena_default_form'] = array(
    '#type' => 'select',
    '#title' => t('Default report skin'),
    '#options' => $skins,
    '#description' => t('Specify the default skin to be used.   New skins can be created by creating .skinfo files in your reports directory.'
    . ' Skins are basically css and javascript libraries added to your report.'),
    '#default_value' => variable_get('forena_default_form', ''),
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

function forena_settings_sync_submit($form, &$form_state) {
  forena_sync_reports($form_state['values']['sync_overwrite']);
  drupal_set_message(t('Report cache cleared'));
}

function forena_format_form($formid, $form_state, $report_name) {
  $desc = Frx::Menu()->parseURL($report_name);
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

    $doclist = Frx::Menu()->doc_formats;
    $skins[''] = t('Use Default');
    $skins = array_merge(variable_get('forena_skins', array()), $skins);

    $form['report_name'] = array(
       '#type' => 'value',
       '#value' => $name,
    );

    $form['form'] = array(
      '#type' => 'select',
      '#title' => t('Skin'),
      '#options' => $skins,
      '#default_value' => $report_form,
      '#description' => t('The page style of your report.  The {skin}.skinfo file specifies css and js file in your report.')
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
      foreach ($doclist as $value) {
        $options[$value] = strtoupper($value);
        $doc =  isset($r) ? $r->simplexml->head->xpath('frx:docgen/frx:doc[@type="' . $value . '"]') : '';
        if ($doc && array_search($value, $doclist)!==FALSE) {
          $default[$value] = $value;
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
    if ($selected = array_filter(@$values['docgen']['docs'])) {
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
function forena_layout_form($form, $form_state, $report_name) {
  $name_in = $report_name;
  $desc = Frx::Menu()->parseURL($report_name);
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
      $menu = $r->getMenu();
      $body = $r->simplexml->body->asXML();
      $css = @(string)$r->simplexml->head->style;

      $form = array();

      //array of xml attributes that are required to have a value
      $required = array('id' => TRUE, 'label' => TRUE);

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
        '#type' => 'text_format',
        '#title' => t('Body'),
        '#default_value' => $body,
        '#rows' => 25,
      '#format' => variable_get('forena_input_format', filter_default_format())
      );
      $form['visibility'] = array(
        '#type' => 'fieldset',
        '#title' => t('Visibility'),
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

      $form['menu'] = array(
        '#type' => 'fieldset',
        '#title' => t('Menu'),
        '#tree' => TRUE,
        '#collapsible' => TRUE,
        '#collapsed' => empty($menu),
      );

      $form['menu']['path'] = array(
        '#type' => 'textfield',
        '#title' => t('Menu Path'),
        '#description' => t('Indicate site reletive path to menu.  Parameters may be embedded in the url using a :parm syntax (e.g. states/:state)'),
        '#default_value' => @$menu['path'],
      );

      $form['menu']['args'] = array(
        '#type' => 'textfield',
        '#title' => t('Additional Arguments'),
        '#description' => t('Indicate additonal parameters that should be extracted after the menu path using a :parm syntax (e.g. :parma/:parmb)'),
        '#default_value' => @$menu['args'],
      );

      $form['menu']['title'] = array(
        '#type' => 'textfield',
        '#title' => t('Menu Title'),
        '#description' => t('Title of menu item.  Leave blank to use the report title as the menu title.'),
        '#default_value' => @$menu['title'],
      );

      $menu_options = array(
        'normal-item' => t('Normal'),
        'local-task' => t('Tab'),
        'default-local-taks' => t('Default Tab'),
        'callback' => t('Callback'),
      );

      $form['menu']['type'] = array(
        '#type' => 'select',
        '#title' => 'Type of menu to create',
        '#options' => $menu_options,
        '#default_value' => @$menu['type'],
      );

      $form['style'] = array(
        '#type' => 'fieldset',
        '#title' => t('CSS Styles'),
        '#collapsible' => TRUE,
        '#collapsed' => TRUE,
      );

      $form['style']['css'] = array(
        '#type' => 'textarea',
        '#default_value' => $css,
        '#description' => t('Specify small css snipets that can be used in the reports.'),
        '#rows' => 10,
        '#cols' => 80,
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

function forena_layout_form_validate($form, &$form_state) {
  $values = $form_state['values'];
  $body = $values['body']['value'];
  $doc_prefix = '<?xml version="1.0" encoding="UTF-8"?>
    <!DOCTYPE root [
    <!ENTITY nbsp "&#160;">
    ]>';
  if ($body) {
    $body_doc = new DOMDocument('1.0', 'UTF-8');
    $body_xml =  $doc_prefix . '<html xmlns:frx="urn:FrxReports">' . $body . '</html>';
    if (@$body_doc->loadXML($body_xml) === FALSE) {
      form_set_error('body', t('Invalid XHTML Document. Check for unclosed tags or stray &'));
    }
  }
  if ($values['menu']['path']) {
    if (!valid_url(str_replace(':', '', $values['menu']['path']), FALSE)) {
      form_set_error('menu][path', t('Invalid Path'));
    }
  }
}

/**
 * builds a string of the xml document,
 * submits it to forena_save_report.
 */
function forena_layout_form_submit($form, &$form_state) {
  $nodes = array();
  $rebuild_menu = FALSE;
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
  $menu = $r->getMenu();
  if ($values['menu']!= $menu) {
    $r->setMenu($values['menu']);
    $rebuild_menu = TRUE;
  }

  // Body
  $r->setBody($values['body']['value']);
  // CSS
  $r->setStyle($values['css']);
  // If there are no frx attributes in the body then replace them with the old values.
  $frx_nodes = $r->simplexml->xpath('body//*[@frx:*]');
  if (!$frx_nodes) {
    $r->save_attributes_by_id($values['attributes']);
  }

  //determine redirection.
  $report_path = forena_report_path();
  $filename = $report_path . '/' . $report_name . '.frx';

  if (forena_save_report($report_name, $r->asXML(), TRUE) == 1) {
    drupal_set_message(t('Your report, %s has been saved.', array('%s' => $report_name)));
  }
  // If we changed the menu we need to rebuild it.
  if ($rebuild_menu) {
    menu_rebuild();
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
  $form_state['redirect'] = array('path' => $link . '/delete', 'query' => array('destination' => $destination));
}


function forena_add_report_form($formid, $form_state, $report_name='') {
  $name = '';
  $filename = '';
  $format = '';
  if ($report_name) {
    $desc = Frx::Menu()->parseURL($report_name);
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
    '#required' => TRUE,
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
  $desc = Frx::Menu()->parseURL($save_report_name);

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
  $desc = Frx::Menu()->parseURL($report_name);
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
    drupal_set_message(t('Your report, %s has been saved.', array('%s' => $report_name)));
    //if this is a new report then redirect to data blocks
    if ($values['report_name']) {
      $edit = '/edit';
    }
    else {
      $edit = '/edit/data/add';
    }
    $form_state['redirect']= $desc['i_link'] . $edit;

  }
}
function forena_create_trans_form($formid, $form_state, $report_name) {
  $name = '';
  $filename = '';
  $format = '';
  $desc = Frx::Menu()->parseURL($report_name);
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
  $lang = @$_GET['language'];
  if ($lang) {
    $language = $languages[$lang];
  }
  $form['base_name'] = array(
    '#type' => 'value',
    '#value' => $base_name
  );
  $form['report_name'] = array(
    '#type' => 'value',
    '#value' => $name,
  );

  foreach ($languages as $key => $obj) {
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
    '#required' => TRUE,
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
  $new_name = $values['save_report_language'] . '/' . $base_name;

  $desc = Frx::Menu()->parseURL($new_name);
  $filename = $desc['filename'];
  $report_name = $desc['name'];

  $r = forena_get_report_editor($values['report_name']);

  // Title and category
  $r->setTitle($values['title']);

  //determine redirection.

  if (file_exists($filename)) {
    drupal_set_message(t('Report %s already exists', array('%s' => $new_name)), 'error');
    return;
  }

  if (forena_save_report($report_name, $r->asXML(), TRUE) == 1) {
    drupal_set_message(t('Translation,  %s has been created. Switch languages to translate.', array('%s' => $values['title'])));
    //if this is a new report then redirect to data blocks
    $form_state['redirect']= array( $desc['i_link'] . '/edit/layout');
  }
}



/*
 * administer the settings for document format options
 */
function forena_doc_formats_settings() {
  // Invoke doc_type hook to see which document types are there.
  $supported_doctypes = Frx::documentTypes();

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
    '#default_value' => variable_get('forena_email_override', FALSE),
    '#description' => t('When this box is checked emails are sent to the currently logged in user.  Useful for testing environments.'),
  );
  $form['forena_email_input_format'] = forena_filter_element(variable_get('forena_email_input_format', 'full_text'), 'forena_email_input_format');
  $form['forena_email_input_format']['#title'] = t('Email Input Format');

  return system_settings_form($form);
}

/*
 * administer the settings for document format options
 */
function forena_data_settings() {
  $repos = Frx::RepoMan()->repositories;
  $r_list = array();
  $headers = array(t('Name'), t('Description'), t('Path'), t('Operation'));
  foreach ($repos as $name => $r) {
    $r_list[] = array(
    $name,
    $r['title'],
    $r['path'],
    l(t('configure'), 'admin/config/content/forena/data/configure/' . $name)
    );

  }

  $output = '<ul class="action-links"><li>' . l(t('Add data source'), 'admin/config/content/forena/data/add') . '</li></ul>';
  $output .= theme_table(array('header' => $headers, 'rows' => $r_list, 'attributes' => array(), 'caption' => '', 'sticky' => TRUE, 'colgroups' => array(), 'empty' => ''));
  return $output;
}

function forena_data_settings_edit($form, &$form_state, $source=-1) {
  global $databases;
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
          'database' => 'default',
          'access callback' => 'user_access',
          'user callback' => 'forena_current_user_id'
          ),
          );
    }
    else {
      Frx::RepoMan()->repository($source);
      $repos = Frx::RepoMan()->repositories;
      $r = $repos[$source];
      // Remove teh object from the data.
      unset($r['data']);
      $form_state['storage'] = array(
        'name' => $source,
        'title' => $r['title'],
        'path' => @$r['path'],
        'config' => $r,
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
    '#required' => TRUE,
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
    '#required' => TRUE,
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
    'block' => t('Match values provided by a data block.'),
  );

  $form['access_method'] = array(
    '#type' => 'select',
    '#options' => $access,
    '#disabled' => $locked,
    '#title' => t('Data security method'),
    '#default_value' => empty($config['access block']) ? 'callback' : 'block',
    '#description' => t('Specify how the ACCESS defined for a data block is to be interpreted.'),
    '#ajax' => array(
      'callback' => 'forena_access_info_callback',
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
        '#autocomplete_path' => 'forena/data_block/autocomplete',
        '#description' => t('The datablock to be used to interpret permissions.  This should return a single column of permissions based on the current user.   May be provided by another repository.'),
        '#default_value' => @$config['access block'],
      );
      break;
    default:
      $form['access_details']['access_callback'] = array(
        '#type' => 'item',
        '#title' => 'Access callback',
        '#disabled' => $locked,
        '#markup' => @$config['access callback'],
      );
  }



  // Driver list
  $drivers = array(
    'FrxDrupal' => t('Drupal'),
    'FrxOracle' => t('Oracle Database'),
    'FrxPDO' => t('PDO other than Drupal'),
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
    '#ajax' => array(
      'callback' => 'forena_connection_info_callback',
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
    '#required' => TRUE,
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
      $db_list = array_combine(array_keys($databases), array_keys($databases));
      $form['connection']['database'] = array(
        '#type' => 'select',
        '#title' => t('Database'),
        '#disabled' => $locked,
        '#default_value' => @$config['database'],
        '#options' => $db_list,
        '#markup' => 'Determined by Drupal settings.php file',
      );
      break;
    case 'FrxMSSQL':
      $form['connection']['uri'] = $uri;
      $form['connection']['user'] = $user;
      $form['connection']['new_password'] = $password;
      $form['connection']['database'] = array(
        '#type' => 'textfield',
        '#disabled' => $locked,
        '#title' => t('Database'),
        '#default_value' => $config['database'],
        '#required' => TRUE,
      );
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
  $result = db_query('SELECT * FROM {forena_repositories} WHERE repository = :name', array(':name' => $name));

  if ($result) drupal_set_message(t('Data connection settings saved'));
  if ($repos = $result->fetchObject()) {
    db_update('forena_repositories')
    ->fields(array(
        'title' => $values['title'],
        'enabled' => $values['enabled'],
        'config' => $config_str,
    ))
    ->condition('repository', $name, '=')
    ->execute();
  }
  else {
    $form_state['#redirect'] = 'admin/forena/content/data';
    db_insert('forena_repositories')
    ->fields(array(
        'repository' => $name,
        'title' => $values['title'],
        'enabled' => $values['enabled'],
        'config' => $config_str,
    ))
    ->execute();
  }
  $form_state['redirect'] = 'admin/config/content/forena/data';
}

/**
 * Delete the function
 * Enter description here ...
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_data_settings_delete($form, &$form_state) {
  if (!@$form_state['storage]']['locked']) {
    $form_state['redirect'] = 'admin/config/content/forena/data';
    db_delete('forena_repositories')
    ->condition('repository', $form_state['values']['name'], '=')
    ->execute();
  }

}

/**
 * Generates the controls for external databases.
 * Enter description here ...
 */
function forena_connection_info_callback($form, $form_state) {
  return $form['connection'];

}

function forena_access_info_callback($form, $form_state) {
  return $form['access_details'];
}


/**
 *
 * @param $form_state
 * @return a form to edit the fields of the report
 */
function forena_fields_form($formid, $form_state, $report_name) {
  $desc = Frx::Menu()->parseURL($report_name);
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
        '#type' => 'vertical_tabs',
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
          '#group' => 'additional_settings',
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

        $form['fields'][$i]['add-query'] = array(
          '#type' => 'checkbox',
          '#title' => t('Pass through url parameters'),
          '#default_value' => @$attr['add-query'],
          '#description' => t('Automatically pass through url parameters on this link'),
        );

        $form['fields'][$i]['rel'] = array(
          '#type' => 'textfield',
          '#title' => t('rel') ,
          '#description' => t('Relationship attribute to apply to the link.'),
          '#size' => 100,
          '#maxlength' => 256,
          '#default_value' => @$attr['rel'],
        );

        $form['fields'][$i]['class'] = array(
          '#type' => 'textfield',
          '#title' => t('class') ,
          '#description' => t('Class applied to be applied to the link.'),
          '#size' => 100,
          '#maxlength' => 256,
          '#default_value' => @$attr['class'],
        );

        $form['fields'][$i]['target'] = array(
          '#type' => 'textfield',
          '#title' => t('target'),
          '#default_value' => @$attr['target'],
          '#size' => 30,
          '#description' => t('Link target eg. _BLANK,  Targets that begin with "popup" will be opened in a new window using javascript.')
        );

        $form['fields'][$i]['default'] = array(
          '#type' => 'textfield',
          '#title' => t('default value'),
          '#default_value' => $default,
          '#size' => 30,
          '#description' => t('The value to be displayed in the report when no value exists.')
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



function forena_add_data_block_form($formid, &$form_state, $report_name) {
  // If no step is set then we're visiting this form for the first time.
  if (!isset($form_state['storage']['step'])) {
    $desc = $form_state['storage']['desc'] = Frx::Menu()->parseURL($report_name);
    $filename = $form_state['storage']['filename'] = $desc['filename'];
    $format = isset($desc['format']) ? $desc['format'] : '';
    $r = forena_get_report_editor($desc['name']);
    $form_state['storage']['title'] = $r->title;
    if (!@$desc['exists']) {
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
      '#required' => TRUE,
      '#description' => t('Enter a data block, a preview will appear below'),
    );

    $form['parameters'] = array(
      '#type' => 'textfield',
      '#title' => t('Parameter data'),
      '#description' => t('Enter parameters to be used for the block as they would appear on a url'),
      '#default_value' => @$form_state['storage']['parm_values'],
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



    $xml = $form_state['storage']['xml'];
    if ($xml) {

      try {
        $xml = @new SimpleXMLElement($xml);
      }
      catch (Exception $e ) {
        drupal_set_message(t('Could not create xml from %s.  Verify that all columns names are valid XML tag names and have no spaces or other special characters.', array('%s' => $form_state['storage']['data_block'])), 'error', FALSE);
      }
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
        $output = forena_render_report($rpt_xml, NULL, $parms, NULL, FALSE);
        //render the xml
        $form_state['storage']['output'] = $output;
        $form_state['storage']['template_body'] = $body;
      }
    }
    $form['template']['back'] = array(
      '#type' => 'submit',
      '#value' => t('Back'),
      '#submit' => array('forena_data_block_back'),
    );
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
        '#markup' => '<p>' . $form_state['storage']['output'] . '</p>',
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

function forena_data_block_back($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['step'] = 'select_block';
}

function forena_data_select_template_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['template'] = $form_state['values']['templates'];
}

function forena_template_info_callback($form, &$form_state) {
  return $form['template'];
}

function forena_add_cancel($form, &$form_state) {
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
  $r = forena_get_report_editor($report_name);
  $head = $r->simplexml->head;

  //Get the xml body of the added data block
  $added_report = $form_state['storage']['template_body'];

  //Get the data block parameters
  $block_info = Frx::RepoMan()->loadBlock($data_block);
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
  if ($data_block) $block_info = Frx::RepoMan()->loadBlock($data_block);
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
  $parameters = $form_state['storage']['parm_data'] = $values['parameters'];
  $parms = isset($values['parms']) ? $values['parms']: array();
  $xml='';
  $block_info='';

  if ($data_block)  {
    //get xml from data block
    //check for and store parameters
    $block_info = Frx::RepoMan()->loadBlock($data_block);
    if ($block_info) {
      //if there were not any parameters passed. Use the report parameters
      if (!$parms) {
        $r = FrxReportGenerator::instance()->get_report($values['report_name']);
        $rpt_params = $r->parameters;
        if ($rpt_params) foreach ($block_info['tokens'] as $index => $id) {
          $parms[$id] = @$rpt_params[$id]['value'];
        }
      }

      drupal_alter('forena_parameters', $name, $parms );


      //now invoke the data provider with the correct params
      Frx::Data()->push($parms, 'parm');
      $form_state['storage']['parms'] = $params;
      $xml = Frx::RepoMan()->data($data_block, $parms);
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
 * @param $formid String representation fo form
 * @param $form_state Array of block type
 * @return unknown_type
 */
function forena_data_block_form($formid, &$form_state, $report_name) {
  $desc = Frx::Menu()->parseURL($report_name);
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
  $links[] = array('href' => $desc['link'] . '/edit/data/add', 'title' => 'Add Data' );
  $form['add_link'] = array(
      '#type' => 'markup',
      '#markup' => theme('links',
  array(
                       'links' => $links,
                       'attributes' => array('class' => 'action-links'),
  )
  )
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
      '#type' => 'vertical_tabs',
  );

  if ($body) foreach ($body->xpath($path) as $node) {
    $attrs = $node->attributes(FRX_NS);
    $id = (string)$node['id'];
    $block = (string)$attrs['block'];
    $parameters = (string)$attrs['parameters'];
    $block_info = Frx::RepoMan()->loadBlock($block);
    $access = @$block_info['access'];
    $data_block_array[$id] = $block . ": <i>" . ' security "' . $access . '"</i>';

    $ctl = array(
        '#type' => 'fieldset',
        '#title' => $id,
        '#group' => 'additional_settings'
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
        '#required' => TRUE,
        '#description' => t('The data block to be used as the source for the data in the report.'),
        );

        $ctl['access'] = array(
        '#type' => 'item',
        '#markup' => $access,
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


function forena_report_translations($name_in='') {
  $desc = Frx::Menu()->parseURL($name_in);
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
  foreach ($languages as $key => $language) {
    $i_name = ($key=='en') ? $name : $key . '/' . $name;
    $i_link = $key . '.' . $name;
    if (file_exists($report_path . '/' . $i_name . '.frx')) {
      $modified = format_date(filemtime($report_path . '/' . $i_name . '.frx'));
      $r = forena_get_report_editor($i_name);
      $title = $r->title;
      $edit = l(t('Edit'), 'reports/' . str_replace('/', '.', $i_link) . '/edit', array('query' => array('language' => $key)));
    }
    else {
      $modified = '';
      $title ='';
      $edit = l(t('Add'), 'reports/' . $name_in . '/translate', array('query' => array('language' => $key)));
    }
    $data[] = array($language->native, $title, $modified, $edit);

  }

  return theme_table(array('header' => $headers, 'rows' => $data, 'attributes' => array(), 'caption' => '', 'sticky' => TRUE, 'colgroups' => array(), 'empty' => ''));
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
 * Form to confirm the delete of a form.
 * @param $form_state
 * @return unknown_type
 */
function forena_delete_form($form_state, $formid, $name_in) {
  // Parse names from url
  $report_name = $name_in;
  $desc = Frx::Menu()->parseURL($report_name);
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
    foreach (language_list() as $language) {
      $report_name = $language->language . '/' . $values['base_name'];
      $desc = Frx::Menu()->parseURL($report_name);
      forena_delete_report($desc['name']);
    }
    drupal_set_message(t('Deleted %s', array('%s' => $values['base_name'])));
  }
  else {
    drupal_set_message(t('Deleted %s', array('%s' => $report_name)));
    forena_delete_report($report_name);
  }
  menu_rebuild();
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
  $repos = Frx::RepoMan()->repositories;
  $blocks= array();
  $user_blocks = array();

  foreach ($repos as $name => $r) {
    // Make really sure the data provider objects have been instantiated
    $o = Frx::RepoMan()->repository($name);
    if (method_exists($o, 'list_blocks')) {
      $blocks = array();
      $o->list_blocks($search, $blocks);
      foreach ($blocks as $block) {
        $block_info = $o->loadBlock($block);
        if (method_exists($o, 'access')) {
          $allow = $o->access(@$block_info['access']);
          if ($allow) $user_blocks[] = $name . '/' . $block;
        }
      }
    }
  }
  if ($user_blocks) asort($user_blocks);
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

function forena_admin_params_form($formid, $form_state, $report_name) {

  $desc = Frx::Menu()->parseURL($report_name);
  if (!$desc['exists']) {
    drupal_not_found();
    exit;
  }
  $name = $desc['name'];
  $filename = $desc['filename'];
  $format = @$desc['format'];
  if ($desc['exists'] && $name) {
    // Set up basic control types.
    $control_types= array('textfield' => 'Text',
      'hidden' => 'Hidden',
      'radios' => 'Radios',
      'checkbox' => 'Checkbox',
      'checkboxes' => 'Checkboxes',
      'select' => 'Select',
      'selectajax' => 'Select with Refresh',
      'multiselect' => 'Multi-Select',
      'multiselectajax' => 'Multi-select With Refresh',
    );
    // Supported module specific control types.
    if (module_exists('date_api')) {
      $control_types['date_select'] =  'Date Select';
    }
    if (module_exists('date_popup')) {
      $control_types['date_popup'] = 'Date Popup';
    }

    //Construct report editor
    $r = forena_get_report_editor($name);
    drupal_set_title($r->title);
    $form = array();

    $form['rpt_name'] = array(
      '#type' => 'value',
      '#value' => $name,
    );

    $links = array();
    $links[] = array('href' => $desc['link'] . '/edit/params/add', 'title' => 'Add Parameter' );
    $form['add_link'] = array(
      '#type' => 'markup',
      '#markup' => theme('links',
        array(
         'links' => $links,
         'attributes' => array('class' => 'action-links'),
        )
      )
    );

    if ($r) $nodes = $r->simplexml->xpath('head/frx:parameters/frx:parm');
    if ($nodes) {
      $form['params'] = array(
        '#tree' => TRUE,
        '#type' => 'vertical_tabs'
        );

        foreach ($nodes as $node) {
          $attrs = $node->attributes();
          $id = @(string)$attrs['id'];
          $label = @(string)$attrs['label'];
          $value = @(string)$attrs['value'] ? (string)$attrs['value'] : (string)$node;
          $require = @(string)$attrs['require'];
          $data_source = @(string)$attrs['data_source'];
          $data_field = @(string)$attrs['data_field'];
          $label_field = @(string)$attrs['label_field'];
          $class = @(string)$attrs['class'];
          $desc = @(string)$attrs['desc'];
          $type = @(string)$attrs['type'];
          $options = @(string)$attrs['options'];

          //make a subtree for each param
          $form['params'][$id] = array(
          '#tree' => TRUE,
          '#type' => 'fieldset',
          '#title' => ($label) ? $label . ' (' . $id . ')' : $id,
          '#collapsible' => TRUE,
          '#collapsed' => TRUE,
          '#group' => 'additional_settings',
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

          $form['params'][$id]['type'] = array(
          '#type' => 'select',
          '#title' => t('Input Control Type'),
          '#default_value' => $type,
          '#options' => $control_types,
          '#description' => t('Enter a helpful description about this parameter. This will display on the form when the user is prompted to enter a parameter.'),
          );

          $form['params'][$id]['data_source'] = array(
          '#type' => 'textfield',
          '#title' => t('Data Source'),
          '#default_value' => $data_source,
          '#autocomplete_path' => 'forena/data_block/autocomplete',
          '#description' => t('Data block to be used for select, multiselect, and checkboxes types'),
          );

          $form['params'][$id]['data_field'] = array(
          '#type' => 'textfield',
          '#title' => t('Data Field'),
          '#default_value' => $data_field,
          '#description' => t('Column in data block to be used data of select boxes and radio buttons. '),
          );

          $form['params'][$id]['label_field'] = array(
            '#type' => 'textfield',
            '#title' => t('Label Field'),
            '#default_value' => $label_field,
            '#description' => t('Column in data block to be used for the label for selet boxes and radio buttons.')
          );

          $form['params'][$id]['options'] = array(
          '#type' => 'textfield',
          '#title' => t('Options'),
          '#default_value' => $options,
          '#description' => t('Used to configure the control type above options depend on the control type above.')
          );

          $form['params'][$id]['class'] = array(
          '#type' => 'textfield',
          '#title' => t('Class'),
          '#description' => t('Specify the html class you wish to apply to this parameter'),
          '#default_value' => $class,
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

function forena_add_param_form($form, &$form_state, $report_name) {
  $desc = Frx::Menu()->parseURL($report_name);
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
  }
}

function forena_admin_params_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  $name = $values['rpt_name'];
  $params = $values['params'];
  // Hack because in rare cases the forms api adds this parmeter.
  unset($params['params__active_tab']);
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
  $result = db_query("SELECT * FROM {forena_reports} where hidden=0 AND category LIKE :category ORDER BY category, title asc",
  array(':category' => $string . '%'));
  $categories = array();
  foreach ($result as $row)  {
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
    if ($access && !isset($categories[$row->category])) {
      $categories[$row->category] = $row->category;
    }
  }
  return $categories;
}

function forena_template_ajax(&$form_element) {
  $form_element['#ajax'] = array(
        'callback' => 'forena_template_info_callback',
        'wrapper' => 'template-wrapper',
  );
}

function forena_query_string($var) {
  return drupal_http_build_query($var);
}


