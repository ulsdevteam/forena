<?php
class FrxParameterForm extends FrxRenderer {
  public $form_id;

  public function render() {
      $form = $this->frxReport->parametersForm();
      $form['#forena_form_template'] = $this->innerXML();
      if (trim($form['#forena_form_template'])) $form['#theme'] = 'forena_inline_form';
      $output = $this->renderForm($form);
      return $output;
  }


}