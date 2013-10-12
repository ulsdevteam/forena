<?php
/**
 * @file FrxEmailMergeDoc.inc
 * email merge document.
 * @author davidmetzler
 *
 */
class FrxEmailMergeDoc extends FrxDocument {
  public function render($r, $format, $options = array()) {
    $body = $r->html;
    $doc = new DOMDocument('1.0', 'UTF-8');
    $doc->strictErrorChecking = FALSE;
    libxml_use_internal_errors(true);
    $doc->loadHTML($body);
    libxml_clear_errors();
    $xml = simplexml_import_dom($doc);
    $docs = $xml->xpath('.//*[@class="email-document"]');
    $prompt_subject = TRUE;
    $prompt_body = TRUE;
    if ($docs) foreach ($docs as $doc) {
      $from = $doc->xpath('.//*[@class="email-header-from"]');
      $from = $from ? (string)$from[0] : '';
      $subject = $doc->xpath('.//*[@class="email-header-subject"]');
      if ($subject) $prompt_subject = FALSE;
      $subject = $subject ? (string)$subject[0] : '';
      $to = $doc->xpath('.//*[@class="email-header-to"]');
      $to = $to ? (string)$to[0] : '';
      if ($to) $prompt_to = FALSE;
      $body = $doc->xpath('.//*[@class="email-body"]');
      if ($body) $prompt_body = FALSE;
      $body = $body ? $body[0]->asXML() : $body;
      $email = array(
        'to' => $to,
        'from' => $from,
        'parms' => array('subject' => $subject,
        'body' => $body),
      );
      $emails [] = $email;
    }
    $count = count($docs);
    if ($count) {
      $form = drupal_get_form('forena_confirm_email', $emails, $count, $prompt_subject, $prompt_body);
      $output = drupal_render($form);
    }
    else {
      drupal_set_message(t('No mail merge information in report. Displaying report instead.'), 'error');
      $output = $body;
    }
    return $output;
  }


}