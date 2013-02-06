<?php
class FrxParameterForm extends FrxRenderer {
  public $form_id;

  public function render() {
      $form = $this->frxReport->parametersForm(TRUE);
      $output = $this->renderForm($form);
      return $output;
  }


}