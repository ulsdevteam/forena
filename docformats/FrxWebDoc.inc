<?php
/**
 * @file FrxWebDoc.inc
 * Standard web document manager
 * @author metzlerd
 *
 */
class FrxWebDoc extends FrxDocument {
  private $title;
  public function render($r, $format, $options=array()) {
    // Set title based on report.
    if ($r->title) {
      $this->title = $r->title;
      //drupal_set_title(filter_xss($r->title));
    }

    // Add Style attribute from header

    // Add css files
    foreach (Frx::Skin()->stylesheets as $type => $sheets) {
      foreach ($sheets as $sheet) {
        switch ($type) {
          case 'all':
          case 'print':
          case 'screen':
            drupal_add_css($sheet, array('media' => $type));
            break;
        }
      }
    }

    // Add inline styles
    if (isset($r->rpt_xml->head->style)) {
      $sheet = (string)$r->rpt_xml->head->style;
      drupal_add_css($sheet, array('type' => 'inline'));
    }

    // Add javascript files
    foreach (Frx::Skin()->scripts as $script) {
      drupal_add_js($script);
    }


    // @TODO: Refactor to support theming of document links.
    // Build Links for document.

    $formats = '';
    if ($r->html) {
      //Building the document links
      $supported_doctypes = Frx::documentTypes();
      $default_doctypes = variable_get('forena_doc_defaults', array());
      $m = Frx::Menu();
      $link = str_replace('/', '.' , $m->base_name);
      if ($m->name) {
        $formats = '<div class="doclinks">';
        if (!$r->formats) {
          $doc_types = $m->doc_defaults;
        }
        else {
          //There were nodes. show the prefered doc types
          $doc_types = array_intersect($m->doc_formats, $r->formats);
        }
        if ($doc_types) foreach ($doc_types as $value) {
          $formats .= @l(strtoupper($value) . ' ', 'report_doc/' . $link . '.' . $value, array('query' => $r->parms, 'class' => 'doclinks'));
        }
        $formats .= '</div>';
      }
    }

    // Use a theme function to make sure the report gets themed.
    $variables = array(
      'doc_links' => $formats,
      'content' => $this->check_markup($r->html),
    );

    $output = theme('forena_web_report', $variables);

    return $output;
  }

  public function output($output) {
    if ($this->title) {
      drupal_set_title(filter_xss($this->title));
    }
    return FALSE;
  }


}