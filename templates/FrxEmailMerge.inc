<?php
/**
 * @file FrxEmailMerge
 * Email merge template.
 */
require_once('FrxTemplate.inc');
class FrxEmailMerge extends FrxTemplate {

  public function config_form($config, $xml='') {
    $form_ctl['from'] = array(
      '#type' => 'textfield',
      '#title' => t('From'),
      '#default_value' => @$config['from'],
    );

    $form_ctl['to'] = array(
      '#type' => 'textfield',
      '#title' => t('To'),
      '#default_value' => @$config['to'],
    );

    $form_ctl['subject'] = array(
      '#type' => 'textfield',
      '#title' => t('Subject'),
      '#default_value' => @$config['subject'],
    );

    $form_ctl['body'] = array(
      '#type' => 'textarea',
      '#title' => t('Message'),
      '#default_value' => @$config['body'],
    );

    return $form_ctl;
  }

  public function generate($xml, $config) {
    $from = $this->extract('from', $config);
    $to = $this->extract('to', $config);
    $subject = $this->extract('subject', $config);
    $body = $this->extract('body', $config);
    $config['foreach'] = "*";
    $div = $this->blockDiv($config);
    $doc = $this->addNode($div, 4, 'div', NULL, array('class' => 'email-document'));
    $header = $this->addNode($doc, 6, 'div', NULL, array('class' => 'email-header'));
    $table = $this->addNode($header, 8, 'table');
    $tr = $this->addNode($table, 10, 'tr');
    $td = $this->addNode($tr, 12, 'th', 'From');
    $td = $this->addNode($tr, 12, 'td', $from, array('class' => 'email-header-from'));
    $tr = $this->addNode($table, 10, 'tr');
    $td = $this->addNode($tr, 12, 'th', 'To');
    $td = $this->addNode($tr, 12, 'td', $to, array('class' => 'email-header-to'));
    $tr = $this->addNode($table, 10, 'tr');
    $td = $this->addNode($tr, 12, 'th', 'Subject');
    $td = $this->addNode($tr, 12, 'td', $subject, array('class' => 'email-header-subject'));
    $email_body = $this->addNode($doc, 6, 'div', NULL, array('class' => 'email-body'));
    $p = $this->addNode($email_body, 8, 'p', $body);
  }
}