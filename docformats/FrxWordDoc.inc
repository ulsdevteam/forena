<?php
/**
 * @file FrxWordDoc.inc
 * Word document exporter.
 * @author davidmetzler
 *
 */
class FrxWordDoc extends FrxDocument {
  public function __construct() {
    $this->content_type = 'application/msword';
  }

  public function render($r, $format, $options = array()) {
    $css = $this->loadCSSFiles($format);
    $output = '<html><head>';
    $output .= '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>';
    if ($options['css']) {
      $output .= '<style type="text/css">';
      $output .= $css;
      $output .= '</style>';
    }
    $output .= '<title>' . $r->title . '</title></head><body class="forena-report">' . $r->html . '</body></html>';
    return $output;
  }
}