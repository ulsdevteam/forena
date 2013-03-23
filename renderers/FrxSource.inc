<?php
/**
 * @file FrxSource
 * Displays source of Frx Report rather than rendering
 * Look at the FrxRender class to see a full list of
 * properties that can be used here.
 */
class FrxSource extends FrxRenderer {
  public function render() {
    $node = $this->reportDocNode;
    $output = "<pre>\n" . htmlspecialchars($node->asXML()) . "\n</pre>";
    return $output;
  }
}