<?php
/**
 * Created by PhpStorm.
 * User: metzlerd
 * Date: 4/21/16
 * Time: 8:32 PM
 */


/**
 * Crosstab Renderer
 *
 * @FrxRenderer(id = "FrxAjax")
 *
 */
class FrxAjax extends FrxRenderer {

  public function render() {
    $text = forena_inner_xml($this->reportDocNode, $this->reportDocNode->getName());
    $command = $this->replacedAttributes();
    $command['text'] = $text;
    unset($command['renderer']);
    $event = $this->extract('event', $command);
    if (strpos($event, 'pre')===0) {
      $event = 'pre';
    }
    else {
      $event = 'post';
    }
    $this->frxReport->addAjaxCommand($command, $event);
    return '';
  }
}