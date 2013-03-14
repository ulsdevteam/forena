<?php
/**
 * @file FrxPrincePDF
 * PDF document via Prince XML
 * @author davidmetzler
 *
 */
class FrxPrincePDF extends FrxDocument {
  private $p;

  public function __construct() {
    include_once('sites/all/libraries/prince/prince.php');
    $this->content_type='application/pdf';
    $prince_path = variable_get('forena_pdf_prince_path', '/usr/local/bin/prince');
    if (class_exists('Prince') && file_exists($prince_path)) {
      $this->p = new Prince($prince_path);
    }

  }


  public function render($r, $format, $options = array()) {

    $disable_links = variable_get('forena_pdf_disable_links', TRUE);
    $link_class = $disable_links ? 'prince-disable-links': '';
    $output = '<html><head>';
    $output .= '<meta http-equiv="Content-Type" content="text/html; charset=UTF-8"/>';
    if (@$options['css']) {
      $output .= '<style type="text/css">';
      $output .= $css;
      $output .= '</style>';
    }

    $output .= '<title>' . $r->title . "</title></head><body class='forena-report $link_class'><h1>" . $r->title . '</h1>' . $this->check_markup($r->html) . '</body></html>';
    $prince_css = drupal_get_path('module', 'forena_pdf') . '/forena_pdf_prince.css';
    // Generate the document
    if ($this->p) {
      $p = $this->p;
      $p->addStyleSheet($prince_css);
      foreach (Frx::Skin()->stylesheets as $type => $sheets) {
        foreach ($sheets as $sheet) {
          switch ($type) {
            case 'all':
            case 'print':
            case 'screen':
            case 'pdf':
              $p->addStyleSheet($sheet);
              break;
          }
        }
      }
      return $output;
    }
    else {
      drupal_set_message(t('Prince XML Not Properly Installed'), 'error');
      return ('');
    }
  }

  public function output($output) {
      header('Content-Type: ' . $this->content_type);
      header('Cache-Control:');
      header('Pragma:');
      header('Cache-Control: must-revalidate');
      $this->p->convert_string_to_passthru($output);
      return TRUE;
  }
}
