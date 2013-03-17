<?php
/**
 * @file
 * paramter form
 * Use to customize report parameters form.
 * @author metzlerd
 *
 */
class FrxParameterForm extends FrxRenderer {
  public function render() {
    $output = '';
    if ($this->format=='web') {
      $variables = $this->replacedAttributes();
      $variables['template']  = $this->innerXML();
      $form = $this->frxReport->parametersForm($variables);
      $output = $this->drupalRender($form);
    }
    return $output;
  }
}