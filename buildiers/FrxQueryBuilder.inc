<?php
/**
 * @file FrxQueryBuilder.inc
 * Base query builder class used for creating data blocks.
 * Classes that extend this one can be used to create data blocks
 * @author metzlerd
 *
 */
class FrxQueryBuilder {
  // The supported plugins for this type
  public $supportedPlugins = array(
  	'FrxDrupal',
    'FrxOracle',
    'FrxMSSQL',
    'FrxPDO',
    'FrxPostgres',
  );

  public $ext = 'sql';

  public function initBlock($block) {

  }

  public function scrapeConfig() {
    return $config();
  }

  public function configForm($config) {

    $form['access'] = array(
        '#title' => t('Data Security'),
        '#description' => t('Value depends on the security for the data block,
            but for drupal data, this is a right as defined by the module provided.  '),
        '#type' => 'textfield',
        '#disabled' => !user_access('forena data security'),
        '#default_value' => @$block['access'],
    );

    $form['file'] = array(
        '#type' => 'textarea',
        '#title' => t('Source'),
        '#default_value' => @$block['file'],
    );

  }

  /**
   * Returns a set of errors keyed by the form elements that
   * Can also be used to alter block configuration.
   * @param $config
   * @param $form_state
   */
  public function validateConfig(&$config, &$form_state) {

  }

  public function generate($config) {
    $file = '';
    $access = trim(@$config['access']);
    if ($access) {
      $file .= "--ACCESS=$access\n";
    }
    if (isset($config['file'])) {
      $file .= $file;
    }
  }
}