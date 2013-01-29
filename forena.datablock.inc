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
    '#submit' => array('forena_query_load_block'),
    '#value' => 'Load',
  );

  $form['access'] = array(
    '#type' => 'item',
    '#title' => 'Access',
    '#markup' => $data['access'],
  );

  $form['source'] = array(
    '#type' => 'textarea',
    '#title' => t('Source'),
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

  $form['submit'] = array('#type' => 'submit', '#value' => 'Preview', '#submit' => array('forena_query_block_preview'));
  $form['preview'] = array('#markup' => @$form_state['storage']['preview']);

  return $form;
}

function forena_query_block_preview($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $data = forena_xml($form_state['values']['block'], @$form_state['values']['parameters']);
  if (is_object($data)) $data = $data->asXML();
  $form_state['storage']['preview'] = htmlspecialchars($data);

}

function forena_query_load_block($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
  $form_state['storage']['block'] = Frx::RepoMan()->loadBlock($form_state['values']['block']);
  $form_state['input']['source'] = $form_state['storage']['block']['source'];
}