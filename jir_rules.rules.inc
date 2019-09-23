<?php

//Implements hook_rules_action_info();
function jir_rules_rules_action_info(){
	$actions = array(
		'jir_rules_create_sms_file' => array(
			'label' => t('Create SMS File'),
			'group' => t('JIR Custom Rules'),
			'parameter' => array(
				'textbody' => array(
					'type' => 'text',
					'label' => t('SMS text'),
					'description' => t('Enter text body to be sent in SMS.'),
				),
				'filename' => array(
					'type' => 'text',
					'label' => t('Filename'),
					'description' => t('Enter text that will be used as filename.'),
				),
				'nodeid' => array(
					'type' => 'integer',
					'label' => t('Job Nid'),
					'description' => t('Job node identifier'),
				),
			),
		),
		'jir_rules_send_sms_files_ftp' => array(
			'label' => t('Send SMS Files to FTP'),
			'group' => t('JIR Custom Rules'),
		),
		'jir_rules_send_user_subscription' => array(
			'label' => t('Send user subscription'),
			'group' => t('JIR Custom Rules'),
			'parameter' => array(
				'email' => array(
					'type' => 'text',
					'label' => t('Email'),
					'required' => TRUE,
				),
			),
		),
		'jir_rules_send_user_unsubscription' => array(
			'label' => t('Send user unsubscription'),
			'group' => t('JIR Custom Rules'),
			'parameter' => array(
				'email' => array(
					'type' => 'text',
					'label' => t('Email'),
					'required' => TRUE,
				),
			),
		),
		'jir_rules_send_application_cv_search' => array(
			'label' => t('Send application to CV Search'),
			'group' => t('JIR Custom Rules'),
			'parameter' => array(
				'date_received' => array(
					'type' => 'date',
					'label' => t('Date Received'),
					'required' => FALSE,
				),
				'firstname' => array(
					'type' => 'text',
					'label' => t('Firstname'),
					'required' => FALSE,
				),
				'lastname' => array(
					'type' => 'text',
					'label' => t('Lastname'),
					'required' => FALSE,
				),
				'email' => array(
					'type' => 'text',
					'label' => t('Email'),
					'required' => FALSE,
				),
				'telephone' => array(
					'type' => 'text',
					'label' => t('Telephone'),
					'required' => FALSE,
				),
				'cover_note' => array(
					'type' => 'text',
					'label' => t('Cover note'),
					'required' => FALSE,
				),
				'nationality' => array(
					'type' => 'text',
					'label' => t('Nationality'),
					'required' => FALSE,
				),
				'diploma' => array(
					'type' => 'text',
					'label' => t('Diploma'),
					'required' => FALSE,
				),
				'study' => array(
					'type' => 'text',
					'label' => t('Field of study'),
					'required' => FALSE,
				),
				'languages' => array(
					'type' => 'list',
					'label' => t('Languages'),
					'required' => FALSE,
				),
				'experience' => array(
					'type' => 'text',
					'label' => t('Experience'),
					'required' => FALSE,
				),
				'sex' => array(
					'type' => 'text',
					'label' => t('Sex'),
					'required' => FALSE,
				),
				'filename' => array(
					'type' => 'text',
					'label' => t('Filename'),
					'required' => FALSE,
				),
				'submission_id' => array(
					'type' => 'integer',
					'label' => t('Submission ID'),
					'required' => FALSE,
				),
			),
		),
	);
	return $actions;
}

//Implements hook_rules_condition_info();
function jir_rules_rules_condition_info(){
	$conditions = array(
		'jir_rules_sms_files_creation_allowed' => array(
			'label' => t('Sms files creation allowed'),
			'group' => t('JIR Custom Conditions'),
		),
	);
	return $conditions;
}

function jir_rules_create_sms_file($textbody, $filename, $nodeid){
	$dir_files = 'public://smsfiles/ready/';
	$actual_message = addslashes($textbody);
	$patterns = array("/\s+/","/\s([?.!])/");
	$replacer = array(" ","$1");
	$clean_message = preg_replace($patterns, $replacer, $actual_message);
	$clean_message = htmlentities($clean_message, ENT_NOQUOTES, 'utf-8');   
	$clean_message = preg_replace('#&([A-za-z])(?:acute|cedil|caron|circ|grave|orn|ring|slash|th|tilde|uml);#', '\1', $clean_message);
	$clean_message = preg_replace('#&([A-za-z]{2})(?:lig);#', '\1', $clean_message);
	$clean_message = preg_replace('#&[^;]+;#', '', $clean_message);
	$done_message = stripslashes($clean_message);
	if(strlen($done_message) <= 171){
		$file = file_save_data($done_message , $dir_files . $filename . '.txt', $replace = FILE_EXISTS_REPLACE);	
		if($file){
			watchdog('SMS debug', 'File saved successfully ', $variables = NULL, $severity = WATCHDOG_NOTICE, $link = NULL);
			$job_node = node_load($nodeid);
			$node_wrapper = entity_metadata_wrapper('node', $job_node);
			$node_wrapper->field_job_sent_sms->set(1);
			$node_wrapper->save();
		}else{
			watchdog('SMS debug', 'File creation failed... ', $variables = NULL, $severity = WATCHDOG_ERROR, $link = NULL);
		}
	}else{
		watchdog('SMS debug', 'File creation skipped for job NID:' . $nodeid . '. Message exceeds 171 chars...', $variables = NULL, $severity = WATCHDOG_ERROR, $link = NULL);
		$job_node = node_load($nodeid);
		$node_wrapper = entity_metadata_wrapper('node', $job_node);
		$node_wrapper->field_job_sent_sms->set(1);
		$node_wrapper->save();
	}
}

//Sends file over ftp
function jir_rules_send_sms_files_ftp(){
	$dir_ready_files = 'public://smsfiles/ready/';
	$dir_sent_files = 'public://smsfiles/sent/';
	$ready_files_count = count_files($dir_ready_files);

	$allowed_files_day = variable_get('number_jobs_per_day');
	$ftp_host = variable_get('sms_jobs_ftp_host');
	$ftp_port = variable_get('sms_jobs_ftp_port');
	$ftp_username = variable_get('sms_jobs_ftp_username');
	$ftp_password = variable_get('sms_jobs_ftp_password');
	$dir_remote = variable_get('sms_jobs_ftp_server_root');

	if ($dh = opendir($dir_ready_files)){
		$conn_id = ftp_connect($ftp_host, $ftp_port);
		watchdog('SMS debug', 'Ftp host: '. $ftp_host.' Ftp username: ' . $ftp_username. ' Ftp password: ' .$ftp_password, $variables = NULL, $severity = WATCHDOG_NOTICE, $link = NULL);
		$login_result = ftp_login($conn_id, $ftp_username, $ftp_password);
		if($conn_id && $login_result){
			watchdog('SMS debug', 'Login passed.', $variables = NULL, $severity = WATCHDOG_NOTICE, $link = NULL);
			//ftp_pasv($conn_id, true); //In case server is virtual
			$files = ftp_nlist($conn_id, '.');
			$count_files = count($files);
			$remotefilescount = 0;
			for($f = 0; $f < $count_files; $f++){
				if(strcasecmp(substr($files[$f], -4), '.txt') !== 0){
					continue;
				}else{
					$remotefilescount++;
				}
			}
			watchdog('SMS debug', 'Files found on remote ' . $remotefilescount, $variables = NULL, $severity = WATCHDOG_NOTICE, $link = NULL);
			while ($remotefilescount < $allowed_files_day && (($file = readdir($dh)) !== false)){
				if(is_dir($dir_ready_files . '/' . $file)){
					continue;
				}
				else{
					if(ftp_put($conn_id, $file, $dir_ready_files . $file, FTP_ASCII)){
						rename($dir_ready_files . $file, $dir_sent_files . $file);
						watchdog('SMS file sent', 'File sent to ftp successfully: ' . $file, $variables = NULL, $severity = WATCHDOG_NOTICE, $link = NULL);
						$headers = 'From: [site:name]<[site:mail]>' . "\r\n" . 'Reply-To: [site:mail]' . "\r\n" . 'X-Mailer: PHP/' . phpversion();
						$email_content = 'An SMS file has been sent over FTP to ' . $ftp_host . '. Content of the file: ' . file_get_contents($dir_sent_files . $file, FILE_USE_INCLUDE_PATH);
						$sitemail = variable_get('site_mail', ini_get('sendmail_from'));
						mail($sitemail, 'JIR SMS alert: ' . $file, $email_content, $headers);
						$remotefilescount++;
					}else{
						watchdog('SMS file not sent.', 'File not sent to ftp: ' . $file, $variables = NULL, $severity = WATCHDOG_ERROR, $link = NULL);
					}	
				}
			}
			watchdog('SMS debug', 'Terminating: ' . $remotefilescount . ' files are ready on remote server.', $variables = NULL, $severity = WATCHDOG_NOTICE, $link = NULL);	
		}else{
			watchdog('SMS debug', 'Login to remote FTP failed', $variables = NULL, $severity = WATCHDOG_ERROR, $link = NULL);
		}
		closedir($dh);
		if($conn_id !== FALSE){
			ftp_close($conn_id);
		}
	}else{
		watchdog('SMS debug', 'Local server sms directory not found.', $variables = NULL, $severity = WATCHDOG_ERROR, $link = NULL);
	}
}

function jir_rules_send_user_subscription($email){
	$query = db_select('simplenews_realname', 'sub')
        ->condition('sub.mail', $email, '=')
        ->fields('sub', array('realname'));
	$query_res = $query->execute()->fetchObject();
	$name_obj = json_encode($query_res);
	$name_dec = json_decode($name_obj, true);
	$name = $name_dec['realname'];

	$options = array(
	    'method' => 'GET',
        'data' => drupal_http_build_query(array('email' => $email, 'fullname' => $name)),
        'timeout' => 15
    );
    global $base_url;
	$domain = substr($base_url, strripos($base_url, 'job'));
	if ($domain !== false && !empty(trim($domain))) {
        $result = drupal_http_request('https://newsletter.' . $domain . '/index.php/addNewsletterSubscriber', $options);
        watchdog('JIU Rules', 'Request: ' . $result->request .
            ' | Result code: ' . $result->code .
            ' | Protocol: ' . $result->protocol .
            ' | Status message: ' . $result->status_message);
    } else {
        watchdog('JIU Rules', 'Base url not found');
    }
}

function jir_rules_send_user_unsubscription($email){
    $options = array(
        'method' => 'GET',
        'data' => drupal_http_build_query(array('email' => $email)),
        'timeout' => 15
    );
    global $base_url;
    $domain = substr($base_url, strripos($base_url, 'job'));
    $result = drupal_http_request('https://newsletter.' . $domain . '/deleteNewsletterSubscriber', $options);
    watchdog('JIU Rules', 'Request: ' . $result->request .
        ' | Result code: ' . $result->code .
        ' | Protocol: ' . $result->protocol .
        ' | Status message: ' . $result->status_message);
}

function jir_rules_sms_files_creation_allowed(){
	$dir_ready_files = 'public://smsfiles/ready/';
	$ready_files_count = count_files($dir_ready_files);
	$allowed_files_day = variable_get('number_jobs_per_day');
	if($ready_files_count < $allowed_files_day){
		return true;
	}else{
		return false;
	}
}

function count_files($dir){
	$count = -2;
	if (is_dir($dir)){
		if ($dh = opendir($dir)){
			while (($file = readdir($dh)) !== false){
				$count++;
			}
			closedir($dh);
		}			
	}
	return $count;
}
/*
		Filename structure
		Array ( [0] => Array ( 
					[fid] => 490 
					[uid] => 0 
					[filename] => a98b712b0c8aa60ead5a3c13c5cc0adc_natkaba2.docx 
					[uri] => 'public://jobapplicationfiles/a98b712b0c8aa60ead5a3c13c5cc0adc_natkaba2.docx' 
					[filemime] => application/vnd.openxmlformats-officedocument.wordprocessingml.document 
					[filesize] => 22862 
					[status] => 1 
					[timestamp] => 1456891903 
					[origname] => NATKABA(2).docx 
					[type] => default 
					[display] => 1 
					[description] => 
					) 
				)
	*/
function jir_rules_send_application_cv_search($date_received, $firstName, $lastName, $email, $tel,
			$covernote, $nationality, $diploma, $study, $languages, $experience, $sex, $filename, $submissionid){
	//Lookup for job details: job id, job title and job category
	$ids = array($submissionid);
	$controller = entity_get_controller('entityform');
	$entities = $controller->load($ids);
	$application = reset($entities);
	$entitywrapper = entity_metadata_wrapper('entityform', $application);
	$job = $entitywrapper->field_job->value();
	$jobid = 0;
	$jobtitle = '';
	$jobcategory = '';
	$job_categories = '';
	if($job !== null){
		$jobid = $entitywrapper->field_job->nid->value();
		$jobid = (int)$jobid;
		$jobtitle = $entitywrapper->field_job->title_field->value();
		$jobcategory = $entitywrapper->field_job->field_category->value();
		foreach($jobcategory as $cat){
			$cat_name = $cat->name;
			$job_categories .= $cat_name;
			$job_categories .= ', ';
		}
		$job_categories = trim($job_categories);
		$job_categories = substr_replace($job_categories ,'',-1);	
	}else{
		/*Unsolicited goes here*/
	}
	//End lookup
	
	//Date received
	$date_received = date('Y-m-d H:i:s', $date_received);
	//First name
	$firstName = $firstName[0]['safe_value'];
	//Last name
	$lastName = $lastName[0]['safe_value'];
	//Email
	$email = $email[0]['email'];
	//Telephone
	$tel = $tel[0]['value'];
	//Cover note: can be empty
	$covernote = empty($covernote) ? '' : strip_tags($covernote[0]['safe_value']);
	//Nationality
	$code = $nationality[0]['iso2'];
	$nationality = country_code_to_country($code);
	//Diploma
	$diploma_tid = (int)$diploma[0]['tid'];
    $diploma = taxonomy_term_load($diploma_tid)->name;
	//Study
	$study_tid = (int)$study[0]['tid'];
	$study = taxonomy_term_load($study_tid)->name;
	//Spoken languages	
	$langs_with_levels = '';
	foreach($languages as $language){
		$lang_id = (int)$language['tid'];
		$lang = taxonomy_term_load($lang_id)->name;
		$level_id = (int)$language['level'];
		$level = '';
		switch($level_id){
			case 1:
				$level = 'None';
				break;
			case 2:
				$level = 'Basic';
				break;
			case 3:
				$level = 'Intermediary';
				break;
			case 4:
				$level = 'Advanced';
				break;
			case 5:
				$level = 'Fluent';
				break;
		}
		$langs_with_levels .= $lang . ':' . $level . ', ';
	}
	$languages = trim($langs_with_levels);
	$languages = substr_replace($languages, '', -1);
	//Experience
	$experience_tid = (int)$experience[0]['tid'];
	$experience = taxonomy_term_load($experience_tid)->name;
	//Sex
	$sex_tid = (int)$sex[0]['tid'];
	$sex = taxonomy_term_load($sex_tid)->name;
	//File name and file hash
	$filename = $filename[0]['filename'];
	$file_hash = '';
	
	if($filename){
		$file_hash .= substr($filename, 0, strpos($filename, '_'));
		$filename = str_replace($file_hash . '_', '', $filename);		
	}
	$candidateProfile = array(
		'DateReceived' => strval($date_received),   // Application date time
		'FirstName' => strval($firstName),
		'LastName' => strval($lastName),
		'Email' => strval($email),
		'Tel' => strval($tel),
		'JobId' => strval($jobid),  // ID of the job  - if unsollicitated, put jobId = 0
		'JobTitle' => strval($jobtitle),
		'JobCategory' => strval($job_categories),
		'CoverNote' => strval($covernote),
		'Nationality' => strval($nationality),
		'Diploma' => strval($diploma),
		'Study' => strval($study),
		'Languages' => strval($languages),
		'Experience' => strval($experience),
		'Sex' => strval($sex),
		'FileName' => strval($filename),
		'FileHash' => strval($file_hash),
	);
	$url_send = 'https://search.jobinrwanda.com/api/candidateprofiles';
	$str_data = json_encode($candidateProfile);

	$ch = curl_init($url_send);
	curl_setopt($ch, CURLOPT_CUSTOMREQUEST, 'POST');
	curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
	curl_setopt($ch, CURLOPT_POSTFIELDS,$str_data);
	//curl_setopt($ch, CURLOPT_FOLLOWLOCATION, 1);
	$result = curl_exec($ch);
	curl_close($ch);  // Seems like good practice
	if($result !== false){
		$ids = array($submissionid);
		$controller = entity_get_controller('entityform');
		$entities = $controller->load($ids);
		$application = reset($entities);
		
		$entitywrapper = entity_metadata_wrapper('entityform', $application);
		$entitywrapper->field_sent_to_search_engine->set(1);
		$entitywrapper->save();
		watchdog('Job application', 'Application submitted successfully: submission id ' . $submissionid, $variables = NULL, $severity = WATCHDOG_NOTICE, $link = NULL);	
	}else{
		watchdog('Job application', 'Application submission failed: submission_id ' .$submissionid, $variables = NULL, $severity = WATCHDOG_ERROR, $link = NULL);	
	}
}

//Returns country name by iso2 code
function country_code_to_country( $code ){
    $country = '';
    if( $code == 'AF' ) $country = 'Afghanistan';
    if( $code == 'AX' ) $country = 'Aland Islands';
    if( $code == 'AL' ) $country = 'Albania';
    if( $code == 'DZ' ) $country = 'Algeria';
    if( $code == 'AS' ) $country = 'American Samoa';
    if( $code == 'AD' ) $country = 'Andorra';
    if( $code == 'AO' ) $country = 'Angola';
    if( $code == 'AI' ) $country = 'Anguilla';
    if( $code == 'AQ' ) $country = 'Antarctica';
    if( $code == 'AG' ) $country = 'Antigua and Barbuda';
    if( $code == 'AR' ) $country = 'Argentina';
    if( $code == 'AM' ) $country = 'Armenia';
    if( $code == 'AW' ) $country = 'Aruba';
    if( $code == 'AU' ) $country = 'Australia';
    if( $code == 'AT' ) $country = 'Austria';
    if( $code == 'AZ' ) $country = 'Azerbaijan';
    if( $code == 'BS' ) $country = 'Bahamas the';
    if( $code == 'BH' ) $country = 'Bahrain';
    if( $code == 'BD' ) $country = 'Bangladesh';
    if( $code == 'BB' ) $country = 'Barbados';
    if( $code == 'BY' ) $country = 'Belarus';
    if( $code == 'BE' ) $country = 'Belgium';
    if( $code == 'BZ' ) $country = 'Belize';
    if( $code == 'BJ' ) $country = 'Benin';
    if( $code == 'BM' ) $country = 'Bermuda';
    if( $code == 'BT' ) $country = 'Bhutan';
    if( $code == 'BO' ) $country = 'Bolivia';
    if( $code == 'BA' ) $country = 'Bosnia and Herzegovina';
    if( $code == 'BW' ) $country = 'Botswana';
    if( $code == 'BV' ) $country = 'Bouvet Island (Bouvetoya)';
    if( $code == 'BR' ) $country = 'Brazil';
    if( $code == 'IO' ) $country = 'British Indian Ocean Territory (Chagos Archipelago)';
    if( $code == 'VG' ) $country = 'British Virgin Islands';
    if( $code == 'BN' ) $country = 'Brunei Darussalam';
    if( $code == 'BG' ) $country = 'Bulgaria';
    if( $code == 'BF' ) $country = 'Burkina Faso';
    if( $code == 'BI' ) $country = 'Burundi';
    if( $code == 'KH' ) $country = 'Cambodia';
    if( $code == 'CM' ) $country = 'Cameroon';
    if( $code == 'CA' ) $country = 'Canada';
    if( $code == 'CV' ) $country = 'Cape Verde';
    if( $code == 'KY' ) $country = 'Cayman Islands';
    if( $code == 'CF' ) $country = 'Central African Republic';
    if( $code == 'TD' ) $country = 'Chad';
    if( $code == 'CL' ) $country = 'Chile';
    if( $code == 'CN' ) $country = 'China';
    if( $code == 'CX' ) $country = 'Christmas Island';
    if( $code == 'CC' ) $country = 'Cocos (Keeling) Islands';
    if( $code == 'CO' ) $country = 'Colombia';
    if( $code == 'KM' ) $country = 'Comoros the';
    if( $code == 'CD' ) $country = 'Congo';
    if( $code == 'CG' ) $country = 'Congo the';
    if( $code == 'CK' ) $country = 'Cook Islands';
    if( $code == 'CR' ) $country = 'Costa Rica';
    if( $code == 'CI' ) $country = 'Cote d\'Ivoire';
    if( $code == 'HR' ) $country = 'Croatia';
    if( $code == 'CU' ) $country = 'Cuba';
    if( $code == 'CY' ) $country = 'Cyprus';
    if( $code == 'CZ' ) $country = 'Czech Republic';
    if( $code == 'DK' ) $country = 'Denmark';
    if( $code == 'DJ' ) $country = 'Djibouti';
    if( $code == 'DM' ) $country = 'Dominica';
    if( $code == 'DO' ) $country = 'Dominican Republic';
    if( $code == 'EC' ) $country = 'Ecuador';
    if( $code == 'EG' ) $country = 'Egypt';
    if( $code == 'SV' ) $country = 'El Salvador';
    if( $code == 'GQ' ) $country = 'Equatorial Guinea';
    if( $code == 'ER' ) $country = 'Eritrea';
    if( $code == 'EE' ) $country = 'Estonia';
    if( $code == 'ET' ) $country = 'Ethiopia';
    if( $code == 'FO' ) $country = 'Faroe Islands';
    if( $code == 'FK' ) $country = 'Falkland Islands (Malvinas)';
    if( $code == 'FJ' ) $country = 'Fiji the Fiji Islands';
    if( $code == 'FI' ) $country = 'Finland';
    if( $code == 'FR' ) $country = 'France, French Republic';
    if( $code == 'GF' ) $country = 'French Guiana';
    if( $code == 'PF' ) $country = 'French Polynesia';
    if( $code == 'TF' ) $country = 'French Southern Territories';
    if( $code == 'GA' ) $country = 'Gabon';
    if( $code == 'GM' ) $country = 'Gambia the';
    if( $code == 'GE' ) $country = 'Georgia';
    if( $code == 'DE' ) $country = 'Germany';
    if( $code == 'GH' ) $country = 'Ghana';
    if( $code == 'GI' ) $country = 'Gibraltar';
    if( $code == 'GR' ) $country = 'Greece';
    if( $code == 'GL' ) $country = 'Greenland';
    if( $code == 'GD' ) $country = 'Grenada';
    if( $code == 'GP' ) $country = 'Guadeloupe';
    if( $code == 'GU' ) $country = 'Guam';
    if( $code == 'GT' ) $country = 'Guatemala';
    if( $code == 'GG' ) $country = 'Guernsey';
    if( $code == 'GN' ) $country = 'Guinea';
    if( $code == 'GW' ) $country = 'Guinea-Bissau';
    if( $code == 'GY' ) $country = 'Guyana';
    if( $code == 'HT' ) $country = 'Haiti';
    if( $code == 'HM' ) $country = 'Heard Island and McDonald Islands';
    if( $code == 'VA' ) $country = 'Holy See (Vatican City State)';
    if( $code == 'HN' ) $country = 'Honduras';
    if( $code == 'HK' ) $country = 'Hong Kong';
    if( $code == 'HU' ) $country = 'Hungary';
    if( $code == 'IS' ) $country = 'Iceland';
    if( $code == 'IN' ) $country = 'India';
    if( $code == 'ID' ) $country = 'Indonesia';
    if( $code == 'IR' ) $country = 'Iran';
    if( $code == 'IQ' ) $country = 'Iraq';
    if( $code == 'IE' ) $country = 'Ireland';
    if( $code == 'IM' ) $country = 'Isle of Man';
    if( $code == 'IL' ) $country = 'Israel';
    if( $code == 'IT' ) $country = 'Italy';
    if( $code == 'JM' ) $country = 'Jamaica';
    if( $code == 'JP' ) $country = 'Japan';
    if( $code == 'JE' ) $country = 'Jersey';
    if( $code == 'JO' ) $country = 'Jordan';
    if( $code == 'KZ' ) $country = 'Kazakhstan';
    if( $code == 'KE' ) $country = 'Kenya';
    if( $code == 'KI' ) $country = 'Kiribati';
    if( $code == 'KP' ) $country = 'Korea';
    if( $code == 'KR' ) $country = 'Korea';
    if( $code == 'KW' ) $country = 'Kuwait';
    if( $code == 'KG' ) $country = 'Kyrgyz Republic';
    if( $code == 'LA' ) $country = 'Lao';
    if( $code == 'LV' ) $country = 'Latvia';
    if( $code == 'LB' ) $country = 'Lebanon';
    if( $code == 'LS' ) $country = 'Lesotho';
    if( $code == 'LR' ) $country = 'Liberia';
    if( $code == 'LY' ) $country = 'Libyan Arab Jamahiriya';
    if( $code == 'LI' ) $country = 'Liechtenstein';
    if( $code == 'LT' ) $country = 'Lithuania';
    if( $code == 'LU' ) $country = 'Luxembourg';
    if( $code == 'MO' ) $country = 'Macao';
    if( $code == 'MK' ) $country = 'Macedonia';
    if( $code == 'MG' ) $country = 'Madagascar';
    if( $code == 'MW' ) $country = 'Malawi';
    if( $code == 'MY' ) $country = 'Malaysia';
    if( $code == 'MV' ) $country = 'Maldives';
    if( $code == 'ML' ) $country = 'Mali';
    if( $code == 'MT' ) $country = 'Malta';
    if( $code == 'MH' ) $country = 'Marshall Islands';
    if( $code == 'MQ' ) $country = 'Martinique';
    if( $code == 'MR' ) $country = 'Mauritania';
    if( $code == 'MU' ) $country = 'Mauritius';
    if( $code == 'YT' ) $country = 'Mayotte';
    if( $code == 'MX' ) $country = 'Mexico';
    if( $code == 'FM' ) $country = 'Micronesia';
    if( $code == 'MD' ) $country = 'Moldova';
    if( $code == 'MC' ) $country = 'Monaco';
    if( $code == 'MN' ) $country = 'Mongolia';
    if( $code == 'ME' ) $country = 'Montenegro';
    if( $code == 'MS' ) $country = 'Montserrat';
    if( $code == 'MA' ) $country = 'Morocco';
    if( $code == 'MZ' ) $country = 'Mozambique';
    if( $code == 'MM' ) $country = 'Myanmar';
    if( $code == 'NA' ) $country = 'Namibia';
    if( $code == 'NR' ) $country = 'Nauru';
    if( $code == 'NP' ) $country = 'Nepal';
    if( $code == 'AN' ) $country = 'Netherlands Antilles';
    if( $code == 'NL' ) $country = 'Netherlands the';
    if( $code == 'NC' ) $country = 'New Caledonia';
    if( $code == 'NZ' ) $country = 'New Zealand';
    if( $code == 'NI' ) $country = 'Nicaragua';
    if( $code == 'NE' ) $country = 'Niger';
    if( $code == 'NG' ) $country = 'Nigeria';
    if( $code == 'NU' ) $country = 'Niue';
    if( $code == 'NF' ) $country = 'Norfolk Island';
    if( $code == 'MP' ) $country = 'Northern Mariana Islands';
    if( $code == 'NO' ) $country = 'Norway';
    if( $code == 'OM' ) $country = 'Oman';
    if( $code == 'PK' ) $country = 'Pakistan';
    if( $code == 'PW' ) $country = 'Palau';
    if( $code == 'PS' ) $country = 'Palestinian Territory';
    if( $code == 'PA' ) $country = 'Panama';
    if( $code == 'PG' ) $country = 'Papua New Guinea';
    if( $code == 'PY' ) $country = 'Paraguay';
    if( $code == 'PE' ) $country = 'Peru';
    if( $code == 'PH' ) $country = 'Philippines';
    if( $code == 'PN' ) $country = 'Pitcairn Islands';
    if( $code == 'PL' ) $country = 'Poland';
    if( $code == 'PT' ) $country = 'Portugal, Portuguese Republic';
    if( $code == 'PR' ) $country = 'Puerto Rico';
    if( $code == 'QA' ) $country = 'Qatar';
    if( $code == 'RE' ) $country = 'Reunion';
    if( $code == 'RO' ) $country = 'Romania';
    if( $code == 'RU' ) $country = 'Russian Federation';
    if( $code == 'RW' ) $country = 'Rwanda';
    if( $code == 'BL' ) $country = 'Saint Barthelemy';
    if( $code == 'SH' ) $country = 'Saint Helena';
 
    if( $code == 'KN' ) $country = 'Saint Kitts and Nevis';
    if( $code == 'LC' ) $country = 'Saint Lucia';
    if( $code == 'MF' ) $country = 'Saint Martin';
    if( $code == 'PM' ) $country = 'Saint Pierre and Miquelon';
    if( $code == 'VC' ) $country = 'Saint Vincent and the Grenadines';
    if( $code == 'WS' ) $country = 'Samoa';
    if( $code == 'SM' ) $country = 'San Marino';
    if( $code == 'ST' ) $country = 'Sao Tome and Principe';
    if( $code == 'SA' ) $country = 'Saudi Arabia';
    if( $code == 'SN' ) $country = 'Senegal';
    if( $code == 'RS' ) $country = 'Serbia';
    if( $code == 'SC' ) $country = 'Seychelles';
    if( $code == 'SL' ) $country = 'Sierra Leone';
    if( $code == 'SG' ) $country = 'Singapore';
    if( $code == 'SK' ) $country = 'Slovakia (Slovak Republic)';
    if( $code == 'SI' ) $country = 'Slovenia';
    if( $code == 'SB' ) $country = 'Solomon Islands';
    if( $code == 'SO' ) $country = 'Somalia, Somali Republic';
    if( $code == 'ZA' ) $country = 'South Africa';
    if( $code == 'GS' ) $country = 'South Georgia and the South Sandwich Islands';
    if( $code == 'ES' ) $country = 'Spain';
    if( $code == 'LK' ) $country = 'Sri Lanka';
    if( $code == 'SD' ) $country = 'Sudan';
    if( $code == 'SR' ) $country = 'Suriname';
    if( $code == 'SJ' ) $country = 'Svalbard & Jan Mayen Islands';
    if( $code == 'SZ' ) $country = 'Swaziland';
    if( $code == 'SE' ) $country = 'Sweden';
    if( $code == 'CH' ) $country = 'Switzerland, Swiss Confederation';
    if( $code == 'SY' ) $country = 'Syrian Arab Republic';
    if( $code == 'TW' ) $country = 'Taiwan';
    if( $code == 'TJ' ) $country = 'Tajikistan';
    if( $code == 'TZ' ) $country = 'Tanzania';
    if( $code == 'TH' ) $country = 'Thailand';
    if( $code == 'TL' ) $country = 'Timor-Leste';
    if( $code == 'TG' ) $country = 'Togo';
    if( $code == 'TK' ) $country = 'Tokelau';
    if( $code == 'TO' ) $country = 'Tonga';
    if( $code == 'TT' ) $country = 'Trinidad and Tobago';
    if( $code == 'TN' ) $country = 'Tunisia';
    if( $code == 'TR' ) $country = 'Turkey';
    if( $code == 'TM' ) $country = 'Turkmenistan';
    if( $code == 'TC' ) $country = 'Turks and Caicos Islands';
    if( $code == 'TV' ) $country = 'Tuvalu';
    if( $code == 'UG' ) $country = 'Uganda';
    if( $code == 'UA' ) $country = 'Ukraine';
    if( $code == 'AE' ) $country = 'United Arab Emirates';
    if( $code == 'GB' ) $country = 'United Kingdom';
    if( $code == 'US' ) $country = 'United States of America';
    if( $code == 'UM' ) $country = 'United States Minor Outlying Islands';
    if( $code == 'VI' ) $country = 'United States Virgin Islands';
    if( $code == 'UY' ) $country = 'Uruguay, Eastern Republic of';
    if( $code == 'UZ' ) $country = 'Uzbekistan';
    if( $code == 'VU' ) $country = 'Vanuatu';
    if( $code == 'VE' ) $country = 'Venezuela';
    if( $code == 'VN' ) $country = 'Vietnam';
    if( $code == 'WF' ) $country = 'Wallis and Futuna';
    if( $code == 'EH' ) $country = 'Western Sahara';
    if( $code == 'YE' ) $country = 'Yemen';
    if( $code == 'ZM' ) $country = 'Zambia';
    if( $code == 'ZW' ) $country = 'Zimbabwe';
    if( $country == '') $country = $code;
    return $country;
}
