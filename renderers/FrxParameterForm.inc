<?php
class FrxParameterForm extends FrxRenderer {
  public $form_id;

  public function render() {
      $variables = $this->mergedAttributes();
      $variables['template']  = $this->innerXML();
      $form = $this->frxReport->parametersForm($variables);
      $output = $this->drupalRender($form);
      return $output;
  }


}