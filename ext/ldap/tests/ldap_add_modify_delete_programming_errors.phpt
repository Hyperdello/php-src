--TEST--
Programming errors (Value/Type errors) for ldap_add(_ext)(), ldap_mod_replace(_ext)(), ldap_mod_add(_ext)(), and ldap_mod_del(_ext)()
--EXTENSIONS--
ldap
--FILE--
<?php

/* ldap_add(_ext)(), ldap_mod_replace(_ext)(), ldap_mod_add(_ext)(), and ldap_mod_del(_ext)() share an underlying C function */
/* We are assuming 3333 is not connectable */
$ldap = ldap_connect('ldap://127.0.0.1:3333');
$valid_dn = "cn=userA,something";

/* Taken from the existing ldap_add() PHP documentation */
$valid_entries = [
    'attribute1' => 'value',
    'attribute2' => [
        'value1',
        'value2',
    ],
];

$empty_dict = [];
try {
    var_dump(ldap_add($ldap, $valid_dn, $empty_dict));
} catch (Throwable $e) {
    echo $e::class, ': ', $e->getMessage(), PHP_EOL;
}

$not_dict_of_attributes = [
    'attribute1' => 'value',
    'not_key_entry',
    'attribute3' => [
        'value1',
        'value2',
    ],
];
try {
    var_dump(ldap_add($ldap, $valid_dn, $not_dict_of_attributes));
} catch (Throwable $e) {
    echo $e::class, ': ', $e->getMessage(), PHP_EOL;
}

$dict_key_with_empty_key = [
    'attribute1' => 'value',
    '' => [
        'value1',
        'value2',
    ],
];
try {
    var_dump(ldap_add($ldap, $valid_dn, $dict_key_with_empty_key));
} catch (Throwable $e) {
    echo $e::class, ': ', $e->getMessage(), PHP_EOL;
}

$dict_key_with_nul_bytes = [
    "attrib1\0with\0nul\0byte" => 'value',
    "attrib2\0with\0nul\0byte" => [
        'value1',
        'value2',
    ],
];
try {
    var_dump(ldap_add($ldap, $valid_dn, $dict_key_with_nul_bytes));
} catch (Throwable $e) {
    echo $e::class, ': ', $e->getMessage(), PHP_EOL;
}

$dict_key_value_not_string = [
    'attribute1' => new stdClass(),
    'attribute2' => [
        'value1',
        'value2',
    ],
];
try {
    var_dump(ldap_add($ldap, $valid_dn, $dict_key_value_not_string));
} catch (Throwable $e) {
    echo $e::class, ': ', $e->getMessage(), PHP_EOL;
}

$dict_key_multi_value_empty_list = [
    'attribute1' => 'value',
    'attribute2' => [],
];
try {
    var_dump(ldap_add($ldap, $valid_dn, $dict_key_multi_value_empty_list));
} catch (Throwable $e) {
    echo $e::class, ': ', $e->getMessage(), PHP_EOL;
}

$dict_key_multi_value_not_list = [
    'attribute1' => 'value',
    'attribute2' => [
        'value1',
        'wat' => 'nosense',
        'value2',
    ],
];
try {
    var_dump(ldap_add($ldap, $valid_dn, $dict_key_multi_value_not_list));
} catch (Throwable $e) {
    echo $e::class, ': ', $e->getMessage(), PHP_EOL;
}

$dict_key_multi_value_not_list_of_strings = [
    'attribute1' => 'value',
    'attribute2' => [
        'value1',
        [],
    ],
];
try {
    var_dump(ldap_add($ldap, $valid_dn, $dict_key_multi_value_not_list_of_strings));
} catch (Throwable $e) {
    echo $e::class, ': ', $e->getMessage(), PHP_EOL;
}

$dict_key_multi_value_not_list_of_strings2 = [
    'attribute1' => 'value',
    'attribute2' => [
        'value1',
        new stdClass(),
    ],
];
try {
    var_dump(ldap_add($ldap, $valid_dn, $dict_key_multi_value_not_list_of_strings2));
} catch (Throwable $e) {
    echo $e::class, ': ', $e->getMessage(), PHP_EOL;
}
/* We don't check that values have nul bytes as the length of the string is passed to LDAP */

?>
--EXPECTF--
ValueError: ldap_add(): Argument #3 ($entry) must not be empty

Warning: ldap_add(): Unknown attribute in the data in %s on line %d
bool(false)

Warning: ldap_add(): Add: Can't contact LDAP server in %s on line %d
bool(false)

Warning: ldap_add(): Add: Can't contact LDAP server in %s on line %d
bool(false)
Error: Object of class stdClass could not be converted to string

Warning: ldap_add(): Add: Can't contact LDAP server in %s on line %d
bool(false)
ValueError: ldap_add(): Argument #3 ($entry) must contain arrays with consecutive integer indices starting from 0

Warning: Array to string conversion in %s on line %d

Warning: ldap_add(): Add: Can't contact LDAP server in %s on line %d
bool(false)
Error: Object of class stdClass could not be converted to string
