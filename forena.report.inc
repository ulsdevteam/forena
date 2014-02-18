<?php
/*
 * @file forena.report.inc
 * Include files containing form function for editinig reports
 */
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

  $r = Frx::Editor($name);
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

  $form['report_name'] = array(
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

  $form['clone_report_name'] = array(
      '#title' => t('Create from report'),
      '#type' => 'textfield',
      '#autocomplete_path' => 'forena/reports/autocomplete',
      '#default_value' => $name,
  );

  $form['save'] = array(
      '#type' => 'submit',
      '#validate' => array('forena_validate_report_name'),
      '#submit' => array('forena_create_report_submit'),
      '#value' => 'Create',
  );
  return $form;

}

function forena_validate_report_name($form, &$form_state) {
  $values = $form_state['values'];
  $pattern = "@^[A-Za-z0-9\/\_]$@";
  $save_report_name = $values['report_name'];
  if (preg_match($pattern, $save_report_name)) {
    form_set_error('report_name', t('Invalid characters in report name') );
  }
  //comparing the report names to see if they have changed.
  //If they have, making sure the new name does not already exist.
  $filename = $save_report_name . '.frx';
  if (Frx::File()->exists($filename, FALSE)) {
    form_set_error('reprot_name', t('The report ' . $save_report_name . ' already exists. Please enter another name.'));
  }
}

function forena_create_report_submit($form, &$form_state) {
  $values = $form_state['values'];
  $report_name = $values['report_name'];
  $desc = Frx::Menu()->parseURL($report_name);
  $report_name = $desc['name'];

  if ($values['report_name']) {
    $r = Frx::Editor($values['clone_report_name']);
    $r->report_name = $values['report_name'];
  }
  else {
    $r = Frx::Editor($values['report_name']);
  }
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

  $r->update();
  //if this is a new report then redirect to data blocks
  if ($values['report_name']) {
    $edit = '/edit';
  }
  else {
    $edit = '/edit/select-data/add-data';
  }
  $form_state['redirect']= $desc['i_link'] . $edit;

}

/**
 * Creates a simple save button to be used with other forms
 * @return multitype:string multitype:string  NULL
 */
function forena_report_save_form($form, &$form_state, $report_name) {
  $form['report_name'] = array(
  	'#type' => 'value',
    '#value' => $report_name,
  );

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#submit' => array('forena_report_save_submit'),
  );

  $form['cancel'] = array(
  	'#type' => 'submit',
    '#value' => t('Cancel'),
    '#submit' => array('forena_report_save_cancel'),
  );
  $parms = $_GET;
  unset($parms['q']);
  $form_state['redirect'] = array("reports/$report_name", array('query' => $parms));
  return $form;
}

function forena_report_save_submit($form, &$form_State) {
  $report_name = $form_state['values']['report_name'];
  $r = Frx::Editor($report_name);
  $r->save();
}

function forena_report_preview($report_name) {
  $r = Frx::Editor($report_name);
  drupal_set_title($r->title);
  $content['save'] = drupal_get_form('forena_report_save_form', $report_name);
  $content['preview'] = $r->preview();
  return $content;
}


/**
 * A form to preview and add data blocks to an existing report
 * @param $formid String representation fo form
 * @param $form_state Array of block type
 * @return unknown_type
 */
function forena_report_data_block_form($formid, &$form_state, $report_name, $action, $block_name, $id='') {
  $config = array();
  $r = Frx::Editor($report_name);
  // List of templates
  $templates = $r->templateOptions();
  if (!isset($form_state['storage'])) {
    $form_state['storage']['id'] = $id;
    $parms = $_GET;
    unset($parms['q']);
    $form_state['storage']['parms'] = $parms;
  }
  $id = isset($form_state['storage']['id']) ? $form_state['storage']['id'] : $id;
  $template_class = isset($form_state['values']['template_class']) ? $form_state['values']['template_class'] : 'FrxTable';
  if (isset($form_state['values']['config'])) {
    $config = $form_state['values']['config'];
  }
  elseif ($id) {
    $template_class = $r->scrapeBlockConfig($id, $config);
  }
  //The default submit handler
  //If someone presses enter, this handler will execute
  $form['action'] = array(
    '#type' => 'value',
    '#value' => $action,
  );

  $form['add'] = array(
    '#type' => 'submit',
    '#value' => 'Add',
    '#submit' => array('forena_report_add_template_submit'),
    '#access' => $action=='add-data',
  );

  $form['update'] = array(
    '#type' => 'submit',
    '#value' => 'Update',
    '#submit' => array('forena_report_edit_template_submit'),
    '#access' => $action=='edit-data',
  );

  $form['prepend'] = array(
    '#type' => 'submit',
    '#value' => 'Insert',
    '#submit' => array('forena_report_prepend_template_submit'),
    '#access' => $action=='prepend-data',
  );

  $form['cancel'] = array(
    '#type' => 'submit',
    '#value' => 'Cancel',
    '#submit' => array('forena_update_cancel'),
  );

  $form['report_name'] = array(
  	'#type' => 'value',
    '#value' => $report_name,
  );

  $form['block_name'] = array(
  	'#type' => 'value',
    '#value' => $block_name,
  );

  $form['id'] = array(
  	'#type' => 'value',
    '#value' => $id,
  );

  $form['template_class'] = array(
    '#type' => 'select',
    '#title' => t('Template'),
    '#options' => $templates,
    '#default_value' => $template_class,
    '#ajax' => array(
  	   'callback' => 'forena_template_callback',
        'wrapper' => 'forena-template-wrapper'
     ),
  );

  $form['template']['#prefix'] = '<div id="forena-template-wrapper">';
  $form['template']['#suffix'] = '</div>';
  $form['template']['config'] = array(
      '#type' => 'fieldset',
      '#title' => 'Settings',
      '#collapsible'=>TRUE,
      "#tree" => TRUE,
  );

  $form['template']['config'] = array_merge($form['template']['config'], $r->templateConfigForm($template_class, $config));
  $form['template']['config']['preview'] = array('#type' => 'button', '#value' => t('Preview'));


  // Generate a preview
  $parms = $form_state['storage']['parms'];
  $r = Frx::Editor('__report_block_preview__');
  $r->setEditorParms($parms);
  $r->addBlock($block_name, $template_class, $config);
  $r->update();
  $preview = $r->preview($parms);
  if (!$preview['#has_data']) {
    $path = "reports/$report_name/edit/preview-data/$action/$block_name";
    if ($id) $path .= "/$id";
    $parms = $_GET;
    unset($parms['q']);
    $options = array();
    if ($parms) $options['query'] = $parms;
    drupal_goto("$path", $options);
  }
  $form['template']['preview']['content'] = $preview['content'];
  $r->cancel();

  return $form;
}

/**
 * Ajax callback to refresh the template data
 * @param unknown $form
 * @param unknown $form_state
 * @return unknown
 */
function forena_template_callback($form, &$form_state) {
  return $form['template'];
}

/**
 * Edit an existing id based on the template.
 * @param unknown $form
 * @param unknown $form_state
 */
function forena_report_edit_template_submit($form, &$form_state){
  extract($form_state['values']);
  $r = Frx::Editor($report_name);
  $parms = $form_state['storage']['parms'];
  $r = Frx::Editor($report_name);
  $r->setEditorParms($parms);
  $r->applyTemplate($id, $template_class, $config);
  $r->update();
  $parms = $_GET;
  unset($parms['q']);
  $form_state['redirect'] = array("reports/$report_name/edit", array('query' => $parms));
}

function forena_report_add_template_submit($form, &$form_state) {
  extract($form_state['values']);
  $parms = $form_state['storage']['parms'];
  $r = Frx::Editor($report_name);
  $r->setEditorParms($parms);
  Frx::Editor($report_name)->addBlock($block_name, $template_class, (array)$config, $id);
  Frx::Editor() ->update();
  $parms = $_GET;
  unset($parms['q']);
  $form_state['redirect'] = array("reports/$report_name/edit", array('query' => $parms));
  }

function forena_report_prepend_template_submit($form, &$form_state) {
  extract($form_state['values']);
  $parms = $form_state['storage']['parms'];
  $r = Frx::Editor($report_name);
  $r->setEditorParms($parms);
  Frx::Editor($report_name)->prependBlock($block_name, $template_class, (array)$config, $id);
  Frx::Editor() ->update();
  $parms = $_GET;
  unset($parms['q']);
  $form_state['redirect'] = array("reports/$report_name/edit", array('query' => $parms));
}

function forena_update_cancel($form, &$form_state) {
  extract($form_state['values']);
  $parms = $_GET;
  unset($parms['q']);
  $form_state['redirect'] = array("reports/$report_name/edit", array('query' => $parms));
  }

function forena_report_save_cancel($form, &$form_state) {
  extract($form_state['values']);
  Frx::Editor($report_name)->cancel();
  $form_state['redirect'] = 'forena';
}

function forena_select_block_list($report_name, $action='add-data', $id='') {
  $header = array(t('Block'), t('Access'), t('Keywords'));
  $blocks = Frx::DataFile()->userBlocks();
  $data=array();

  foreach ($blocks as $block_name => $block) {
    $blck = str_replace('/', '.', $block_name);
    $path = "reports/$report_name/edit/preview-data/$action/$blck";
    // Build the table.
    if ($id) $path .= "/$id";
    $data[] = array(
      l($block_name, $path),
      @$block->cache['access'],
      @$block->cache['keywords']
    );
  }

  // Theme the table
  $content['data_blocks'] = array('#markup' => theme('forena_data_table', array('header' => $header, 'rows' => $data)));
  return $content;
}

/**
 * Preview the block
 * @param unknown $report_name
 * @param unknown $block_name
 * @param string $id
 * @return multitype:string unknown multitype:string unknown  multitype:string  multitype:string Ambigous <The, string, A, Optional>
 */
function forena_report_block_select_form($form, &$form_state, $report_name, $action, $block_name, $id='') {
  $form['action'] = array(
      '#type' => 'value',
      '#value' => $action,
  );

  $form['report_name'] = array(
  	'#type' => 'value',
    '#value' => $report_name,
  );

  $form['block_name'] = array(
  	'#type' => 'value',
    '#value' => $block_name,
  );

  $form['id'] = array(
  	'#type' => 'value',
    '#value' => $id,
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Select'),
  );
  return $form;
}

function forena_report_block_select_form_submit($form, &$form_state) {
  //Instantiate form variables
  extract($form_state['values']);
  $parms = $_GET;
  unset($parms['q']);
  $path = "reports/$report_name/edit/$action/$block_name";
  if ($id) $path = "$path/$id";
  $form_state['redirect'] = array($path, array('query' => $parms));
}
/**
 * Page to allow selection of a data block with parameters.
 * @param unknown $report_name
 * @param unknown $block_name
 * @param string $id
 * @return Ambigous <unknown, multitype:multitype:NULL  Ambigous <string, multitype:multitype:multitype:string   multitype:multitype:string unknown   > Ambigous <The, boolean, string> >
 */
function forena_report_block_preview($report_name, $action, $block_name, $id='') {
  $parms = $_GET;
  unset($parms['q']);
  $preview = forena_data_block_preview($block_name, $parms);
  if (@trim($preview['content']['#markup'])) {
    $content['select'] = drupal_get_form('forena_report_block_select_form', $report_name, $action, $block_name, $id);
  }
  else {
    drupal_set_message(t('Sample data is required in order to correctly generate a template.  Try changing parameters'), 'warning');
  }
  $content['preview'] = $preview;
  return $content;
}




