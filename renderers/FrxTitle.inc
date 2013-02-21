<?php
/**
 * @file
 * Implements a title renderer
 * @author metzlerd
 *
 */
class FrxTitle extends FrxRenderer {
  public function render() {
    $html = $this->innerXML();
    $html = $this->teng->replace($html);
    $text = check_plain($html);
    $this->frxReport->title = $text;
    return '';

  }
}