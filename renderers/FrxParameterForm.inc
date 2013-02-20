<?php
class FrxParameterForm extends FrxRenderer {
  public $form_id;

  public function render() {
      $form = $this->frxReport->parametersForm($this->innerXML());
      $output = $this->drupalRender($form);
      return $output;
  }


}