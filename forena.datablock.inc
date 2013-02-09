<?php
/**
 * @file forena.datablock.inc
 * Data block building and testing forms.
 */
require_once 'FrxDataSource.inc';
function forena_query_edit_block_form($form, &$form_state) {

  // Extract off the arguemnts for loading the block.
  $data = @$form_state['storage']['block'];

  $form['block'] = array(
    '#type' => 'textfield',
    '#autocomplete_path' => 'forena/data_block/autocomplete',
    '#title' => 'Data Block',
    '#required' => TRUE,
    '#description' => t('The data block to be used as the source for the data in the report.'),
  );

  $form['load'] = array(
    '#type' => 'submit',
    '#submit' => array('forena_query_load_block', 'forena_query_block_preview'),
    '#value' => 'Load',
  );
  // Only allow save as of sql data blocks.
  if ($data && $data['type']=='sql') {
    if (!@$data['locked']) {
      $form['save'] = array(
        '#type' => 'submit',
        '#value' => 'Save',
        '#submit' => array('forena_query_save_existing_block')
      );
    }
    $form['submit'] = array('#type' => 'submit', '#value' => 'Preview', '#submit' => array('forena_query_block_preview'));

    $form['save_as'] = array('#type' => 'fieldset', '#title' => 'Save As', '#collapsed' => TRUE, '#collapsible' => TRUE);
    $form['save_as']['save_as_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Name'),
      '#description' => t('Indicate a proper path to save the data.  '),
    );

    $form['save_as']['save_as_submit'] = array(
      '#type' => 'submit',
      '#validate' => array('forena_query_validate_new_block'),
      '#submit' => array('forena_query_save_new_block'),
      '#value' => t('Save As')
    );
  }

  $form['access'] = array(
    '#type' => 'item',
    '#title' => 'Access',
    '#markup' => $data['access'],
  );

  $form['source'] = array(
    '#type' => 'textarea',
    '#title' => t('Source'),
    '#disabled' => @$data['locked'],
    '#default_value' => @$data['source'],
  );


  if (@$data['tokens']) {

    $form['parameters'] = array(
      '#title' => 'Parameters',
      '#type' => 'fieldset',
      '#tree' => TRUE
    );

    foreach ($data['tokens'] as $name) {
      $ctl = array(
        '#type' => 'textfield',
        '#title' => check_plain($name),
      );
      $form['parameters'][$name] = $ctl;
    }
  }

  $form['preview'] = array('#markup' => @$form_state['storage']['preview']);

  return $form;
}

function forena_query_block_preview($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $block = $form_state['values']['block'];
  $block_def = isset($form_state['storage']['block']) ? $form_state['storage']['block'] : array();
  $sql = $form_state['values']['source'];
  list($provider, $block_name) = @explode('/', $block, 2);
  $new_block = Frx::RepoMan()->sqlBlock($provider, $sql);
  if ($new_block) $block_def = array_merge($block_def, $new_block);
  $form_state['storage']['block'] = $block_def;
  $parms = isset($form_state['values']['parameters']) ? $form_state['values']['parameters'] : array();
  FrxReportGenerator::instance()->alter_parameters('', $parms);
  $data = Frx::RepoMan()->sqlData($provider, $sql, $parms);
  $t = Frx::Template('FrxTable');
  $config = array('class' => 'dataTable', 'id' => 'sqldata');
  if ($data && $t) {
    $t->generate($data, $config);
    $report = $t->asXML();
    $rpt = new FrxReport($report, $data);
    $form_state['storage']['preview'] = $rpt->render('web');
  }
  if (is_object($data)) $data = $data->asXML();
  //$form_state['storage']['preview'] = htmlspecialchars($data);

}

function forena_query_load_block($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['block'] = Frx::RepoMan()->loadBlock($form_state['values']['block']);
  $form_state['storage']['block_name'] = $form_state['values']['block'];
  $form_state['input']['source'] = $form_state['values']['source'] = $form_state['storage']['block']['source'];
}

function forena_query_validate_new_block($form, &$form_state) {

  // Check to make sure a filename is specified.

  $block_name = $form_state['values']['save_as_name'];
  if (!$block_name) {
    form_set_error('save_as_name',  t("Block_name is required!"));
    return;
  }

  list($src_repos, $name) = @explode('/', $form_state['values']['block']);
  list($dest_repos, $name) = @explode('/', $block_name);
  if ($src_repos != $dest_repos) {
    form_set_error('save_as_name', t('Block must be in the same data source/repository'));
    return;
  }

  $block = Frx::RepoMan()->loadBlock($block_name);
  if ($block) {
    form_set_error('save_as_name', t('Block already exists'));
    return;
  }
}

/**
 * Save one block as another.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_save_new_block($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $block_name = $form_state['values']['save_as_name'];
  $data = $form_state['storage']['block'];
  $form_state['storage']['block']['locked']=FALSE;
  Frx::RepoMan()->saveBlock($block_name, $data);
  $form_state['input']['block'] = $block_name;
  $form_state['storage']['block_name'] = $block_name;
  drupal_set_message('Block Saved');
}

function forena_query_save_existing_block($form, &$form_state) {
  $block_name = $form_state['storage']['block_name'];
  $data = $form_state['storage']['block'];
  $data['source'] = $form_state['values']['source'];
  Frx::RepoMan()->saveBlock($block_name, $data);
  drupal_set_message('Block Saved');
  $form_state['input']['block'] = $block_name;
  $form_state['rebuild'] = TRUE;
}

