<?php
require_once('FrxTemplate.inc');
class FrxEmailMerge extends FrxTemplate {
	public function generate($xml, $config) {
		$from = $this->extract('from', $config);
		$to = $this->extract('to', $config);
		$subject = $this->extract('subject', $config);

		$div = $this->blockDiv($config);
		$doc = $this->addNode($div, 4, 'div', NULL, array('class' => 'email-document'));
		$header = $this->addNode($doc, 6, 'div', NULL, array('class' => 'email-header'));
		$table = $this->addNode($header,8, 'table');
		$tr = $this->addNode($table, 10, 'tr');
		$td = $this->addNode($tr, 12, 'th', 'From');
		$td = $this->addNode($tr, 12, 'td', $from, array('class' => 'email-header-from'));
		$tr = $this->addNode($table, 10, 'tr');
		$td = $this->addNode($tr, 12, 'th', 'To');
		$td = $this->addNode($tr, 12, 'td', $to, array('class' => 'email-header-to'));
		$tr = $this->addNode($table, 10, 'tr');
		$td = $this->addNode($tr, 12, 'th', 'Subject');
		$td = $this->addNode($tr, 12, 'td', $subject, array('class' => 'email-header-subject'));
		$body = $this->addNode($doc, 6, 'div', NULL, array('class' => 'email-body'));
		$p = $this->addNode($body, 8, 'p');
	}
}