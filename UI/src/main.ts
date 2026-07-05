import { createApp } from 'vue'
import App from './App.vue'

import './assets/css/theme.css'
import './UtilsScripts/initBridge.ts'//注册juce监听器

createApp(App).mount('#app')
