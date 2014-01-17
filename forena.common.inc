<?php
// $Id$
/**
 * @file
 * Common functions used throughout the project but loaded in this
 * file to keep the module file lean.
 */
// Include Report renderer.
require_once 'Frx.inc';
require_once 'FrxReport.inc';
require_once 'FrxDocument.inc';
require_once('FrxDataSource.inc');
require_once('FrxReportGenerator.inc');
require_once('FrxSQLQueryBuilder.inc');
require_once('FrxDrupalApplication.inc');
require_once('FrxData.inc');

/**
 * Build cache
 *
 * @param xhtml $r_xhtml Forena XML report.
 * @return array data to be stored in the cache field for the report in the database.
 */
function forena_load_cache($r_xhtml) {
  $conf=array();
  $cache = array();
  $blocks = array();
  $repos = array();
  $nodes = array();
  if (is_object($r_xhtml)) {
    $head = @$r_xhtml->head;
    if ($head) $nodes = $head->xpath('frx:menu');
    $menu = array();
    if ($nodes) {
      $m = $nodes[0];
      $attrs = $m->attributes();
      foreach ($attrs as $key => $value) {
        $menu[$key] = (string)$value;
      }
    }
    if ($menu) $cache['menu'] = $menu;
    $block_xml = $r_xhtml->xpath('//*[@frx:block]');
    // Extract all the blocks and organize by provider

    foreach ($block_xml as $key => $block_node) {
      $attrs = $block_node->attributes('urn:FrxReports');
      foreach ($attrs as $key => $value) {
        if ($key == 'block') {
          @list($provider, $block) = explode('/', $value, 2);
          $repos[$provider][] = $block;
        }
      }
    }

    if ($repos) foreach ($repos as $provider => $blocks) {
      $provider = Frx::RepoMan()->repository($provider);
      if (isset($provider->conf))$conf = $provider->conf;
      $access = array();

      foreach ($blocks as $block_name) {
        if ($provider && $block_name) {
          if (method_exists($provider, 'loadBlock')) {
            $conf = $provider->conf;
            $block = $provider->loadBlock($block_name);
            if (isset($block['access']) && array_search($block['access'], $access)===FALSE) $access[]=$block['access'];
          }
        }
        else {
          //drupal_set_message('no provider found', 'error');
        }
      }

      if (isset($conf['access callback']) && $access) $cache['access'][$conf['access callback']]=$access;

    }



  }
  return $cache;
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

function forena_get_user_reports() {
  GLOBAL $language;

  $result = db_query('SELECT * FROM {forena_reports} where hidden=0 and language=:language ORDER BY category,title asc', array(':language' => $language->language));
  $reports = array();
  foreach ($result AS $row) {
    $access = TRUE;
    $cache = $row->cache;

    if ($cache) {
      $cache = unserialize($cache);

      // Check each callback function to see if we have an error.
      if (@$cache['access']) foreach ($cache['access'] as $callback => $args) {
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
  foreach ($reports as $category => $reports) {
    $output .= '<li>' . l($category, 'forena', array('fragment' => urlencode($category))) . '</li>';
  }
  $output .= '</ul>';
  return $output;
}

/**
 * Email confirmation form.   Confirms an email send based on mail merge
 * @param array $docs An array of SimpleXML email documents to send
 * @param integer $count Number of documents to send.
 */
function forena_confirm_email($formid, &$form_state, $docs, $count, $prompt_subject, $prompt_body) {
  // Save arguments away for rebuild
  if (!isset($form_state['storage']['args'])) {
    $form_state['storage']['args'] = array(
      'docs' => $docs,
      'count' => $count,
      'prompt_subject' => $prompt_subject,
      'prompt_body' => $prompt_body,
    );
  }
  else {
    // Retrieve arguements for rebuild case
    extract($form_state['storage']['args']);
  }
  if ($docs) {
    $values = @$form_state['values'];
    if ($prompt_subject) {
      $form['subject'] = array(
        '#type' => 'textfield',
        '#title' => t('Subject'),
        '#default_value' => @$values['subject'],

      );
    }

    if ($prompt_body) {
      $form['body'] = array(
        '#type' => 'text_format',
        '#title' => t('Message'),
        '#default_value' => @$values['body'],
        '#format' => variable_get('forena_email_input_format', filter_default_format())

      );
    }


    if (!variable_get('forena_email_override', FALSE)) {
      $form['send'] = array(
        '#type' => 'radios',
        '#title' => t('Send Email'),
        '#options' => array('send' => 'email to users',
          'test' => 'emails to me (test mode)'),
        '#default_value' => 'test',
      );
    }
    $form['max'] = array(
      '#type' => 'textfield',
      '#title' => 'Only send first',
      '#description' => 'In test mode only, limits the number of messages to send',
      '#default_value' => 1,
      '#size' => 3,
    );

    $form_state['storage']['docs'] = $docs;
    $form_state['storage']['count'] = $count;
  }
  return confirm_form($form, t('Send mail to users'), 'forena', t('Send email to %count users?', array('%count' => $count)));
}

function forena_confirm_email_submit($form, &$form_state) {
  global $user;
  $test_send = @$form_state['values']['send']=='test' ? TRUE : variable_get('forena_email_override', FALSE);
  $max = (integer)$form_state['values']['max'];
  $i = 0;
  if (isset($form_state['values']['body']['value'])) {
    $body =  check_markup($form_state['values']['body']['value'],$form_state['values']['body']['format']);
  }
  foreach ($form_state['storage']['docs'] as $doc) {
    $to = $test_send ? $user->mail : $doc['to'];
    $from = $doc['from'];
    if (isset($form_state['values']['body'])) {
      $doc['parms']['body'] = $body;
    }
    if (isset($form_state['values']['subject'])) {
      $doc['parms']['subject'] = $form_state['values']['subject'];
    }
    if ($test_send) $i++;
    if ($i <= $max) drupal_mail('forena', 'mailmerge', $to, language_default(), $doc['parms'], $from, TRUE);
  }
  $form_state['rebuild'] = TRUE;
}

/**
 * Send report block XML
 * Enter description here ...
 * @param unknown_type $block_name
 */
function forena_block_xml($block_name='') {
  $block_name = str_replace('.', '/', $block_name);
  $parms = $_GET;
  unset($parms['q']);
  $xml = Frx::RepoMan()->data($block_name, $parms);
  if (is_object($xml)) {
    header('Content-Type: text/xml');
    print $xml->asXML();
  }
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
      FrxReportGenerator::instance()->app->error(t('Could not save %s because XML was malformed', htmlspecialchars($report_name)),
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
  $name = $data['name'];
  $filepath = $report_path . '/' . $report_name . '.frx';
  // If we need to save this to the file system

  if ($save_file) {
    if (!file_exists($filepath) || is_writeable($filepath)) {
      // Serialize the report for saving
      file_put_contents($filepath, $r_xml);

    }
    else {
      Frx::error(t('Insufficient privileges to write file.'));
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
    @list($tlang, $tname) = explode('/', $name, 2);
    if (array_key_exists($tlang, language_list())) {
      $lang = $tlang;
      $name = $tname;
    }
  }


  // Save to the Database
  if (file_exists($filepath)) {
    $modified = filemtime($filepath);
    $result = db_query("SELECT report_name FROM {forena_reports} WHERE report_name=:report_name AND language=:language", array(':report_name' => $name, 'language' => $lang) );
    $path = isset($cache['menu']) ? @$cache['menu']['path'] : '';
    if ($rpt = $result->fetchObject() ) {
      db_update('forena_reports')
      -> condition('report_name', $name)
      -> condition('language', $lang)
      ->fields(array(
          'title' => $data['title'],
          'descriptor' => $data['descriptor'],
          'category' => $data['category'],
          'hidden' => $data['hidden'],
          'cache' => $rpt_cache,
          'altered' => $data['altered'],
          'modified' => $modified,
          'path' => $path))
      ->execute();
    }
    else {
      db_insert('forena_reports')->
      fields( array('report_name' => $name,
          'language' => $lang,
          'title' => $data['title'],
          'descriptor' => $data['descriptor'],
          'category' => $data['category'],
          'hidden' => $data['hidden'],
          'cache' => $rpt_cache,
          'altered' => $data['altered'],
          'modified' => $modified,
          'path' => $path,
      ))
      ->execute();
    }

    $save_count++;
  }

  $r = NULL;
  $result = NULL;
  $r_xml = NULL;
  $report = NULL;
  $data = NULL;
  $rpt = NULL;
  $cache = NULL;
  $rpt_cache = NULL;
  // Destroy object to clear memory leaks.
  if ($r) {
    $r->__destruct();
    unset($r);
  }

  return $save_count;
}

/**
 * Render a field without it's containers so that we can use it in a report without the wrappers.
 */
function theme_forena_inline_field(&$variables) {
  $element = $variables['field'];
  // This is also used in the installer, pre-database setup.
  drupal_render_children($variables['field']);
}

/**
 * Render a special form with an embedded forena report.
 * @param unknown_type $variables
 */
function theme_forena_inline_form($variables) {
  $form = $variables['form'];
  $build = '';
  $output = '';
  $fields = array();
  $template = @$form['#forena_form_template'];

  // Convert interior elements into inline fields
  if ($template) {
    _forena_set_inline_theme($form, $template);
    // Render the inline fields and store them into the array.
    $build = _forena_render_inline_elements($form, $fields, $template);
    $build .= $fields['form_build_id'] . $fields['form_token'] . $fields['form_id'];
  }
  else {
    $build = $form['#children'];
  }

  // Return the form
  return $build;
}

function _forena_render_form_template($fields, $template) {
  static $teng = '';
  if (!$teng) $teng = new FrxSyntaxEngine(FRX_TOKEN_EXP, '{}');
  $o = Frx::Data()->push($fields, 'form-fields');
  $build = $teng->replace($template, TRUE);
  $o = Frx::Data()->pop();

  return $build;
}

function _forena_set_inline_theme(&$elements) {
  foreach ($elements as $key => $value) if (strpos($key, '#')===FALSE) {
    $type = @$value['#type'];
    // Set theme functions for specific types to be inline forms.
    switch ($type) {
      case 'fieldset':
      case NULL:
      case 'submit':
      case '';
      break;
      default:
        $elements[$key]['#theme_wrappers'] = array('forena_inline_form_element');
    }

  }
}

/**
 * Renders forena reprot as
 * @param unknown_type $variables
 */
function theme_forena_inline_form_element($variables) {
  $element = &$variables['element'];
  // This is also used in the installer, pre-database setup.
  $t = get_t();

  // This function is invoked as theme wrapper, but the rendered form element
  // may not necessarily have been processed by form_builder().
  $element += array(
    '#title_display' => 'before',
  );

  // Add element #id for #type 'item'.
  if (isset($element['#markup']) && !empty($element['#id'])) {
    $attributes['id'] = $element['#id'];
  }
  // Add element's #type and #name as class to aid with JS/CSS selectors.
  $attributes['class'] = array('form-item');
  if (!empty($element['#type'])) {
    $attributes['class'][] = 'form-type-' . strtr($element['#type'], '_', '-');
  }
  if (!empty($element['#name'])) {
    $attributes['class'][] = 'form-item-' . strtr($element['#name'], array(' ' => '-', '_' => '-', '[' => '-', ']' => ''));
  }
  // Add a class for disabled elements to facilitate cross-browser styling.
  if (!empty($element['#attributes']['disabled'])) {
    $attributes['class'][] = 'form-disabled';
  }
  $output = '';
  $output = '<span' . drupal_attributes($attributes) . '>' . "\n";
  //drupal_set_message('<pre>'. print_r($element,1) . '</pre>');

  $element['#title_display'] = 'none';

  $prefix = isset($element['#field_prefix']) ? '<span class="field-prefix">' . $element['#field_prefix'] . '</span> ' : '';
  $suffix = isset($element['#field_suffix']) ? ' <span class="field-suffix">' . $element['#field_suffix'] . '</span>' : '';

  switch ($element['#title_display']) {
    case 'before':
    case 'invisible':
      $output .= ' ' . theme('form_element_label', $variables);
      $output .= ' ' . $prefix . $element['#children'] . $suffix . "\n";
      break;

    case 'after':
      $output .= ' ' . $prefix . $element['#children'] . $suffix;
      $output .= ' ' . theme('form_element_label', $variables) . "\n";
      break;

    case 'none':
    case 'attribute':
      // Output no label and no required marker, only the children.
      $output .= ' ' . $prefix . $element['#children'] . $suffix . "\n";
      break;
  }

  // No descriptions or titles.
  /*
  if (!empty($element['#description'])) {
  $output .= '<div class="description">' . $element['#description'] . "</div>\n";
  }
  */
  $output .= "</span>\n";

  return $output;
}

function theme_forena_fieldset_template(&$variables) {

  $element = $variables['fieldset'];
  element_set_attributes($element, array('id'));
  _form_set_class($element, array('form-wrapper'));
  $output = '';

  $fields = array();
  foreach ($element as $key => $value ) if (strpos($key, '#') ===FALSE) {
    $fields[$key] = drupal_render($value);
  }

  $output .= _forena_render_form_template($fields, $element['#forena-template']);
  if (isset($element['#value'])) {
    $output .= $element['#value'];
  }


  return $output;
}


