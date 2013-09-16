<?php

/**
 * Special report embedder
 * @author davidmetzler
 *
 */
class FrxReportContext {

  // Get report based on current context and embed it.
  public function getValue($key, $context) {
    $output = forena_report($key, NULL, FALSE);
    return $output;
  }

  // Undo setter, no possibility to override.
  public function setValue($value) {

  }
}