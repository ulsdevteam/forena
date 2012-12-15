<?php
/**
 * @file
 * bean plugin for with a forena report
 */

class FrxBean extends BeanPlugin {
  public function values() {
    $values = array(
      'settings' => array(
        'selected_report' => FALSE,
      ),
    );
    return array_merge(parent::values(), $values);
  }

  public function form($bean, $form, &$form_state) {
    $form = array();
    $form['settings'] = array(
      '#type' => 'fieldset',
      '#tree' => 1,
      '#title' => t('Settings'),
    );
    // ask politely which node to show
    $form['settings']['selected_report'] = array(
      '#type' => 'textfield',
      '#title' => t('Select a report'),
      '#autocomplete_path' => 'forena/reports/autocomplete',
      '#default_value' => isset($bean->settings['selected_report']) ? $bean->settings['selected_report'] : 0 ,
      '#required' => TRUE,
      '#multiple' => FALSE,
    );

    return $form;
  }


  public function view($bean, $content, $view_mode = 'default', $langcode = NULL) {
    $report = $bean->settings['selected_report']; 
    $output = forena_report_include($report); 
    return array('content' => array('#markup' => $output)); 
  }
}

