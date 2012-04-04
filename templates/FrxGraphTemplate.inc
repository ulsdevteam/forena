<?php
class FrxGraphTemplate extends FrxTemplate {

	public function config_form($config, $xml='') {
		$form_ctl = array();

		$graph_types = array(
		  'bargraph' => 'Bar Graph',
		  'piechart' => 'Pie Chart',
		  'scatterplot' => 'Scatter Plot',
		);

    $styles = array(
      '' => 'Normal',
      '3D' => '3D',
    );

    if ($config['type']=='bargraph') $styles = $styles + array('grouped' => 'Grouped', 'stacked' => 'Stacked', 'horizontal' => 'Horizontal');

    $form_ctl['type'] = array(
      '#type' => 'select',
      '#title' => t('Graph Type'),
      '#options' => $graph_types,
      '#default_value' => $config['type'],
      '#ajax' => array(
        'callback' => 'forena_template_info_callback',
        'wrapper' => 'template-wrapper',
      ),
    );

   $form_ctl['style'] = array(
      '#type' => 'select',
      '#title' => t('Style'),
      '#options' => $styles,
      '#disabled' => ($config['type'] == 'scatterplot'),
      '#default_value' => $config['style']
    );


    $form_ctl['link'] = array(
    );

		$form_ctl['label'] = array(
		  '#type' => 'textfield',
		  '#title' => t('Label'),
		  '#default_value' => $config['label'],
		);

		$form_ctl['series'] = array(
		  '#type' => 'fieldset',
		  '#title' => t('Series'),
		);

		$num_series = 4;
		for ($i=1; $i <= $num_series; $i++) {
			$form_ctl['series'][$i] = array(
			  '#type' => 'textfield',
			  '#title' => $i,
			  '#default_value' => $config['series'][$i],
			);
		}

		return $form_ctl;
	}

	public function generate($xml, $config) {
		$media = $this->extract('media', $config);
		$media = $media ? $media : 'FrxSVGGraph';
		$div = $this->blockDiv($config);
		if ($media == 'FrxSVGGraph') {
			// Clean series
			foreach($config['series'] as $i=>$series) if (!$series) unset($config['series'][$i]);
			if (count($config['series'])==1) $config['series'] = $config['series'][1];
			// Clean colors
			if (isset($config['colors'])) foreach($config['colors'] as $i=>$color) if (!$color) unset($color[$i]);
			$type = $this->extract('type', $config);
			$xpath = $this->extract('xpath', $config);
			$clause = $this->extract('clause', $config);
			$frxparms = array(
			  'type' => $type,
			  'renderer' => 'FrxSVGGraph',
			  'xpath' => $xpath,
			  'options' => drupal_http_build_query($config)
			);

			$svg = $this->addNode($div, 2, 'svg', null, null, $frxparms);
		}
	}

}