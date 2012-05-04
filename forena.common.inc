<?php
/**
 * @file
 * Common functions used throughout the project but loaded in this
 * file to keep the module file lean.
 */
// Include Report renderer.
require_once('FrxReport.inc');
require_once('FrxDataProvider.inc');
require_once('FrxReportGenerator.inc');
require_once('FrxSQLQueryBuilder.inc');
require_once('FrxDrupalApplication.inc');
require_once('FrxData.inc');
/**
 * Get name from argument 1 or alterntaively from a file name
 *
 * @param unknown_type $name
 */
function forena_report_desc(&$name='') {
	global $language;
	$lang = $language->language;
	$base_name = $name;
  if (module_exists('locale')) {
  	//First check to see if the report allready has a language in it
	  @list($tlang,	$tbase_name) = explode('.', $name, 2);
	  // FInd out if the starting name of the report is an installed language.
	  $lang_list = language_list();
	  if (array_key_exists($tlang, $lang_list )) {
	  	$base_name = $tbase_name;
	  	if ($lang != $tlang) {
	      $lang = $tlang;
	      $language = $lang_list[$lang];
	  	}
	  	if ($tlang == 'en') {
	  		$name = $tbase_name;
	  		$base_name = $tlang . '.' . $tbase_name;
	  	}
	  }
	  else {
	    if ($lang != 'en') $name = $lang . '.' . $name;
	  }
  }
  $desc = FrxReportGenerator::instance()->report_desc($name);
  $desc['language'] = $lang;
  $desc['base_name'] = $base_name;
  return $desc;
}


function forena_load_cache($r_xhtml) {
  $conf=array();
  $cache = array();
  $blocks = array();
  $repos = array();
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
      $repos = FrxReportGenerator::instance()->repository($provider);
      $provider = $repos['data'];
      if (isset($provider->conf))$conf = $provider->conf;
      $access = array();

      foreach ($blocks as $block_name) {
        if ($provider && $block_name) {
          if (method_exists($provider, 'load_block')) {
            $conf = $provider->conf;
            $block = $provider->load_block($block_name);
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
function forena_parameter_form(&$form_state, $r) {

  $parms = $_GET;
  unset($parms['q']);
  $form = array();

  if ($r) {
      drupal_set_title($r->title);
      $head = $r->rpt_xml->head;


      $form['params'] = array(
        '#tree' => TRUE,
        '#title' => t('Parameters'),
        '#type' => 'fieldset',
        '#collapsible' => TRUE,
        '#collapsed' => (string)$r->blocks_loaded,
      );

      $nodes = $head->xpath('frx:parameters/frx:parm');
      if ($nodes) {

	      foreach ($nodes as $node) {
	        $label = (string)$node['label'];
	        $id = (string)$node['id'];
	        $data_source = (string)$node['data_source'];
	        $data_field = (string)$node['data_field'];

	        $type = (string)$node['type'];
	        if (isset($parms[$id])) {
	        	$value = (string)$parms[$id];
	        	$multi_value=(array)$value;
	        }
	        else {
	        	$value = (string)$node;
	        	$multi_value = array();
		        if (strpos($value, '|')!==FALSE) $multi_value = explode('|', $value);
	        }
	        $desc =  (string)$node['desc'];
	        $label_field = (string)$node['label_field'];

	        (strcmp((string)$node['require'], "1") == 0) ? $required = TRUE : $required = FALSE;

	        //Determine the form element type to be displayed
	        //If select or radios is chosen then begin a $list array for display values.
	        $multiselect = FALSE;
	        $list = array();
	        switch ($type) {
	        	case 'multiselect':
	        		$type = 'select';
	        		$multiselect = TRUE;
	        		$value = $multi_value;
	        		break;
	        	case 'checkboxes':
	        		$value = $multi_value;
	          case 'select':
	          case 'radios':

	           break;
	          default:
	           $type = 'textfield';
	           $list = '';
	        }

	        //If a data_source attr was found then create an array of
	        //returned values filtered against data_field attr.
	        if ($data_source) {
	          $list = FrxReportGenerator::instance()->data_block_params($data_source, $data_field, $label_field);
	          if (!$required) $list = array('' => '') + $list;
	        }

	        $form['params'][$id] = array(
	          '#type' => $type,
	          '#title' => ($label) ? t($label) : t($id),
	          '#default_value' => $value,
	          '#required' => $required,
	          '#description' => t($desc),
	        );

	        //if $list is not empty then push options
	        //onto the array. options will cause an error for
	        //textfield elements.
	        if ($type != 'textfield') {
	          $form['params'][$id]['#options'] = $list;
	          $form['params'][$id]['#multiple'] = $multiselect;
	        }

	      }

	      $form['params']['submit'] = array(
	        '#type' => 'submit',
	        '#value' => t('Submit'),
	      );

      }
    }
	  return $form;
}

/**
 *
 * gets the values from the params form
 * redirects to the report page with the values in
 * the querystring.
 */
function forena_parameter_form_submit($form, &$form_state) {
  $values = $form_state['values'];
  foreach($values['params'] as $key=>$value) {
  	if (is_array($value)) {
  		foreach ($value as $k => $val) {
  			if (!$val) {
	  			unset($values['params'][$key][$k]);
  			}
  		}
  	}
  	else {
  	  if (strpos($value, '|')!==FALSE) {
  	    $values['params'][$key] = explode('|', $value);
  		}
  	}
  }
  $form_state['redirect']= array($_GET['q'], array('query' => $values['params']));
}

function forena_get_user_reports() {
	GLOBAL $language;
  $result = db_query("SELECT * FROM {forena_reports} where hidden=0 and language='%s' ORDER BY category,title asc", $language->language);
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

/**
 * Email confirmation form.   Confirms an email send based on mail merge
 * @param array $docs An array of SimpleXML email documents to send
 * @param integer $count Number of documents to send.
 */
function forena_confirm_email(&$form_state, $docs, $count) {
  if ($docs) {
  	if (!variable_get('forena_email_override', FALSE)) {
    	$form['send'] = array(
    	  '#type' => 'radios',
    	  '#title' => 'Send Email',
    	  '#options' => array('send' => 'email to users',
    	    'test' => 'emails to me (test mode)'),
    	  '#required' => TRUE,
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
	$test_send = $form_state['values']['send']=='test' ? TRUE : variable_get('forena_email_override', FALSE);
  $max = (integer)$form_state['values']['max'];
	$i = 0;
  foreach ($form_state['storage']['docs'] as $doc) {
  	$to = $test_send ? $user->mail : $doc['to'];
  	if ($test_send) $i++;
    if ($i <= $max) drupal_mail('forena', 'mailmerge', $to, language_default(), $doc['parms'], $doc['from'], TRUE);
  }
}
/**
 * Helper function to process java callbacks.
 */
function forena_ahah_helper_start() {
  // The form is generated in an include file which we need to include manually.
  // We're starting in step #3, preparing for #4.
  $form_state = array('storage' => NULL, 'rebuild' => TRUE);
  $form_build_id = $_POST['form_build_id'];
  // Step #4.
  $form = form_get_cache($form_build_id, $form_state);

  // Preparing for #5.
  $args = $form['#parameters'];
  $form_id = array_shift($args);
  $form_state['post'] = $form['#post'] = $_POST;
  $form['#programmed'] = $form['#redirect'] = FALSE;

  // Step #5.
  $form_state['reuild'] = TRUE;
  drupal_process_form($form_id, $form, $form_state);

  // Step #6 and #7 and #8.
  $form = drupal_rebuild_form($form_id, $form_state, $args, $form_build_id);
	return $form;
}
/**
 * Helper function to render output forms
 * @param array $form
 */
function forena_ahah_helper_end(&$form) {
  unset($form['#prefix'], $form['#suffix']);
  $output = theme('status_messages') . drupal_render($form);

  // Final rendering callback.
  drupal_json(array('status' => TRUE, 'data' => $output));
}

