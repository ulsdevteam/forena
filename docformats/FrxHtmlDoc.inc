<?php
class FrxHtmlDoc extends FrxDocument { 
  
  
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
    print $output;
  }
}