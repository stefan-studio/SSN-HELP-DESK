document.getElementById('ticketForm').addEventListener('submit', function(e) {
    e.preventDefault();

    const ticketData = {
        block: document.getElementById('block').value,
        room: document.getElementById('room').value,
        category: document.getElementById('category').value,
        description: document.getElementById('description').value,
        priority: document.querySelector('input[name="priority"]:checked').value
    };

    console.log("Data captured for C Engine:", ticketData);
    alert("Ticket Submitted to SSN Helpdesk Queue!");
    // Later, we can use 'fetch' to send this to a local server
});