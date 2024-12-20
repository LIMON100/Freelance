import { useEffect } from 'react';
import Head from 'next/head';

const DialogflowWidget = () => {
  useEffect(() => {
    if (typeof window !== 'undefined') {
    // Ensure the script is added only once to avoid conflicts
    const script = document.createElement('script');
    script.src = 'https://www.gstatic.com/dialogflow-console/fast/df-messenger/prod/v1/df-messenger.js';
    script.async = true;
    document.body.appendChild(script);

    return () => {
      document.body.removeChild(script);
    };
}}, []);

  return (
    <>
      <Head>
        <link
          rel="stylesheet"
          href="https://www.gstatic.com/dialogflow-console/fast/df-messenger/prod/v1/themes/df-messenger-default.css"
        />
      </Head>
      <df-messenger
        project-id="ai-chat-441308"
        agent-id="656ccaa9-7ebe-49ef-996f-c22c025d452b"
        language-code="en"
        max-query-length="-1"
        style={{
          zIndex: 999,
          position: 'fixed',
          bottom: '16px',
          right: '16px',
          '--df-messenger-font-color': '#000',
          '--df-messenger-font-family': 'Google Sans',
          '--df-messenger-chat-background': '#E7EFFE',
          '--df-messenger-message-user-background': '#d3e3fd',
          '--df-messenger-message-bot-background': '#fff',
        }}
      >
        <df-messenger-chat-bubble chat-title="Call Companion" />
      </df-messenger>
    </>
  );
};

export default DialogflowWidget;
