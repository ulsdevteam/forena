<?php
/**
 * @file forena.datablock.inc
 * Data block building and testing forms.
 */
require_once 'FrxDataSource.inc';
require_once 'forena.data.inc';

/**
 * Create new sql data form hook
 * @param unknown $form
 * @param unknown $form_state
 * @return unknown
 */
function forena_query_create_block_form($form, &$form_state, $provider='') {
  $providers = Frx::RepoMan()->repositories;
  $form['provider'] = array(
    '#required' => TRUE,
    '#title' => t('Data Source'),
    '#type' => 'select',
    '#options' => Frx::RepoMan()->listRepos(),
    '#default_value' => $provider,

  );

  $form['block_name'] = array(
  	'#type' => 'forena_machine_name',
    '#required' => TRUE,
    '#title' => t('Block to create'),
  );

  $form['add'] = array(
  	'#type' => 'submit',
    '#value' => t('Create'),
  );
  return $form;
}

/**
 * Create a new block.
 * @param unknown $form
 * @param unknown $form_state
 */
function forena_query_create_block_form_validate($form, &$form_state) {
  extract ($form_state['values']);
  $pattern = "@^[A-Za-z0-9\/\_]$@";
  if (preg_match($pattern, $block_name)) {
    form_set_error('block_name', t('Invalid characters in block name') );
  }
  else {
    $path = $provider . '/' . $block_name;
    $block = Frx::RepoMan()->loadBlock($path);
    if ($block) form_set_error('block_name', t('Data Block already exists'));
  }
}

/**
 * Submit handler that creates the block
 * @param unknown $form
 * @param unknown $form_state
 */
function forena_query_create_block_form_submit($form, &$form_state) {
  extract($form_state['values']);
  $path = $provider . '/' . $block_name;
  Frx::BlockEditor($path)->update(
  	array('type' => 'sql',
  	      'access' => '',
  	      'file' => '',
  	      'source' => '',
  	)
  );
  $form_state['redirect'] = 'admin/structure/forena/data/' . str_replace('/', '.', $path) . "/edit";
}

/**
 * Basic SQL Editor form hook implementation
 * @param unknown $form
 * @param unknown $form_state
 * @param string $block_name
 * @return multitype:NULL
 */
function forena_query_editor($form, &$form_state, $block_name='') {
  if (!isset($form_state['storage'])) {
    $form_state['storage']['block_name'] = $block_name;
    $form_state['storage']['parameters'] = array();
  }
  $block_name = $form_state['storage']['block_name'];
  $o = Frx::BlockEditor($block_name);
  $block = $o->block;
  $builder = isset($block['builder']) ? $block['builder'] : 'FrxQueryBuilder';
  $form['block_label'] = array(
    '#type' => 'item',
    '#title' => t('Query (data block) name'),
    '#markup' => str_replace('.', '/', $block_name),

  );

  $builders = $o->builderList();

  $form['builder'] = array(
    '#type' => 'select',
    '#title' => t('Template'),
    '#default_value' => $builder,
  );
  $form['config'] = $o->configForm($builder, $config);
  $form['config']['#tree'] = 'true';
  $form['config']['#prefix'] = "<div id='query-builder-config'>\n";
  $form['config']['#suffix'] = '</div>';


  $form['save'] = array(
      '#type' => 'submit',
      '#value' => 'Save',
      '#validate' => array('forena_query_validate_block'),
      '#submit' => array('forena_query_editor_update', 'forena_query_save_block')
  );
  $form['cancel'] = array('#type' => 'submit', '#value' => t('Cancel'), '#submit' => array('forena_query_close'), '#limit_validation_errors' => array());


  $tokens = $o->tokens();
  if ($tokens) {
    $form['parameters'] = array(
        '#title' => 'Parameters',
        '#type' => 'fieldset',
        '#tree' => TRUE
    );

    foreach ($tokens as $name) {
      $ctl = array(
          '#type' => 'textfield',
          '#title' => check_plain($name),
      );

      $form['parameters'][$name] = $ctl;
    }
  }

  $form['submit'] = array('#type' => 'submit', '#value' => t('Test Changes'), '#submit' => array('forena_query_editor_update', 'forena_query_preview'));
  $form['preview'] = @$form_state['storage']['preview'];
  return $form;
}

function forena_query_validate_block($form, &$form_state) {
   if (isset($form_state['values']['access'])) {
     $o = Frx::BlockEditor();
     $o->load($form_state['storage']['block_name']);
     $r = $o->repos();
     $access = $r->access($form_state['values']['access']);
     if (!$access) {
       form_set_error('access', t('You must be granted access to use this right.'));
     }
   }
   if (!Frx::DataFile()->isWritable(Frx::BlockEditor()->block_name . '.sql')) {
     form_set_error('block', t('Insufficient Permission to save file'));
   }


}

function forena_query_preview($form, &$form_state) {
  $form_state['rebuild'] = TRUE;

  $parms = $form_state['storage']['parameters'];
  if (isset($form_state['values']['parameters'])) {
    $parms = $form_state['storage']['parameters'] = $form_state['values']['parameters'];
  }
  $preview = forena_data_block_preview($form_state['storage']['block_name'], $parms, TRUE);
  $form_state['storage']['preview'] = $preview['content'];
}

function forena_query_delete_form($form, &$form_state, $block_name) {
  $block_name = str_replace('.', '/', $block_name);
  $o =  Frx::BlockEditor($block_name);
  $block = $o->block;
  $file = @$block['file'];
  if ($o->modified) {
    $o->cancel();
    drupal_not_found($block_name);
    exit;
  }

  $form_state['storage']['block_name'] = $block_name;

  $form['block_title'] = array('#markup' => "<h2>$block_name</h2>");

  $form['file'] = array('#markup' => "<pre>$file</pre>");

  $form['confirm'] = array(
    '#type' => 'checkbox',
    '#title' => t('Are you sure you want to delete this block?'),
    '#required' => TRUE,
  );

  $form['block_name'] = array(
  	'#type' => 'value',
    '#value' => $o->block_name,
  );

  $form['delete'] = array(
  	'#type' => 'submit',
    '#value' => t('Delete'),
    '#submit' => array('forena_query_delete_block', 'forena_query_close'),
  );
  $form['cancel'] = array('#type' => 'submit', '#value' => t('Cancel'), '#submit' => array('forena_query_close'), '#limit_validation_errors' => array());
  return $form;
}


function forena_query_save_block($form, &$form_state) {
  $form_state['rebuild'] = FALSE;
  Frx::BlockEditor()->update($form_state['values'])->save();
}

function forena_query_delete_block($form, &$form_state) {
  $block_name = $form_state['values']['block_name'];
  Frx::RepoMan()->deleteBlock($block_name);
  $form_state['redirect'] = 'admin/structure/forena/data';
  drupal_set_message(t('Deleted %s', array('%s' => $block_name)));
}

function forena_query_close($form, &$form_state) {
  Frx::BlockEditor($form_state['storage']['block_name'])->cancel();
  $form_state['rebuild'] = FALSE;
  $form_state['redirect'] = 'admin/structure/forena/data';

}

/**
 * Function to allow all normal button submits to be for the form.
 * @param unknown_type $form
 * @param unknown_type $form_state
 */
function forena_query_editor_update($form, &$form_state) {
  // Instantiate form values
  $o = Frx::BlockEditor($form_state['storage']['block_name']);
  Frx::BlockEditor()->update($form_state['values']);
  $form_state['rebuild'] = TRUE;
}



