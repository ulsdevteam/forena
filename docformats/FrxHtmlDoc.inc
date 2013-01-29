<?php
/**
 * @file FrxHtmlDoc
 * Straight html document with no wrapping theme.
 * @author davidmetzler
 *
 */
class FrxHtmlDoc extends FrxDocument {
  public function render($r, $format, $options = array()) {
    $css = $this->loadCSSFiles($format);
    $output = '<html><head>';
    $output .= '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>';
    $title = $r->title;
    if (@$options['css']) {
      $output .= '<style type="text/css">';
      $output .= $css;
      $output .= '</style>';
    }
    $output .= '<title>' . $r->title . '</title></head><body class="forena-report"><h1>' . $r->title . '</h1>' . $r->html . '</body></html>';
    return $output;
  }

  public function output($output) {
    print $output;
  }
}