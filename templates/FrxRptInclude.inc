<?php
/**
 * @file FrxRptInclude.inc
 * Generate an include of another report.
 * @author davidmetzler
 *
 */
class FrxRptInclude extends FrxTemplate {


  public function generate($xml, $config) {
    $src = $this->extract('src', $key);

    $div = $this->blockDiv($config);
    $frx = array('src' => $src, 'renderer' => 'FrxInclude');
    $this->addNode($div, 4, 'div', NULL, $config, $frx);
  }
}