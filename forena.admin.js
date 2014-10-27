/**
 * @file
 * Legacy forena behaviors.  These are deprecated. 
 */
(function ($) {

  Drupal.behaviors.forenaAdmin = {
    attach: function (context, settings) {
      $('table.dataTable-paged').dataTable({
        "sPaginationType": "full_numbers" 
      }); 
     
    }
  };

})(jQuery);
