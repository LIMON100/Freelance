from flask import Flask, request, jsonify
from flask_cors import CORS

app = Flask(__name__)
CORS(app)

@app.route('/api/chat', methods=['POST'])
def chat():
    user_message = request.json.get('message')
    prev_messages = request.json.get('prevMessages')

    # Process the message using your langchain setup
    resultAIChatbot = chatBot(user_message, prev_messages)
    bot_response = {"role": "assistant", "content": resultAIChatbot}

    return jsonify(bot_response)

if __name__ == '__main__':
    app.run(port=5002)