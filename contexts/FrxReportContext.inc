<?php

/**
 * Special report embedder
 * @author davidmetzler
 *
 */
class FrxReportContext extends FrxContextBase {

  // Get report based on current context and embed it.
  public function getValue($key) {
    $output = forena_report_include($key);
    return $output;
  }

  public function __get($key) {
    return $this->getValue($key);
  }
}