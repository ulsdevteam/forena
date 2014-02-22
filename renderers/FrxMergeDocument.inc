<?php
class FrxMergeDocument extends FrxRenderer {
  public $templateName = 'Generic Merge Document';

  /**
   * Returns the section
   * Enter description here ...
   */
  public function configForm($config, $xml='') {
    $form_ctl = array();
    $form_ctl['content'] = array(
        '#type' => 'text_format',
        '#title' => t('Document'),
        '#rows' => 5,
        '#default_value' => @$config['content'],
       // '#ajax' => $this->configAjax('blur')
    );

    return $form_ctl;
  }

  public function configValidate(&$config) {
    $this->validateTextFormats($config, array('content'));
  }

  public function scrapeConfig() {
    $config = array();
    $this->extractTemplateHTML($this->reportDocDomNode, $content);
  }

  public function generate($xml, $config) {
    $div = $this->blockDiv($config);
    // $config['foreach'] = '*';
    $config['class'] = get_class($this);
    $this->addFragment($div, $config['content']);

  }
}