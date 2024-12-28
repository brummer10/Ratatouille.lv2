
function (event, funcs)
{
    function check_neural_model(icon, value)
    {
        if (value == 'None')
            icon.find ('[rata-role=SlotA]').text ('-- choose a NAM/AIDA-X model --');
    }
    function check_neural_model1(icon, value)
    {
        if (value == 'None')
            icon.find ('[rata-role=SlotB]').text ('-- choose a NAM/AIDA-X model --');
    }
    function check_irfile(icon, value)
    {
        if (value == 'None')
            icon.find ('[rata-role=IrA]').text ('-- choose an IR file --');
    }
    function check_irfile1(icon, value)
    {
        if (value == 'None')
            icon.find ('[rata-role=IrB]').text ('-- choose an IR file --');
    }

    if (event.type == 'start')
    {
    }
    else if (event.type == 'change')
    {
        if (event.uri == 'urn:brummer:ratatouille#Neural_Model')
            check_neural_model(event.icon, event.value);
        else if  (event.uri == 'urn:brummer:ratatouille#Neural_Model1')
            check_neural_model1(event.icon, event.value);
        else if  (event.uri == 'urn:brummer:ratatouille#irfile')
            check_irfile(event.icon, event.value);
        else if  (event.uri == 'urn:brummer:ratatouille#irfile1')
            check_irfile1(event.icon, event.value);

    }
}
